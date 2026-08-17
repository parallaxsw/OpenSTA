// OpenSTA, Static Timing Analyzer
// Copyright (c) 2026, Silimate, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
// 
// The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software.
// 
// Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 
// This notice may not be removed or altered from any source distribution.

#include "DbFile.hh"

#include <fstream>

#include "Error.hh"
#include "Format.hh"
#include "Zlib.hh"

namespace sta {

// Size of a single table of contents entry: id, offset, stored size, raw size
// and checksum.
static constexpr size_t stadb_toc_entry_size = 4 + 8 + 8 + 8 + 4;

uint32_t
dbChecksum(const uint8_t *data, size_t size)
{
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < size; i++) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  return hash;
}

uint64_t
dbDigest(uint64_t seed, const void *data, size_t size)
{
  const uint8_t *bytes = static_cast<const uint8_t*>(data);
  uint64_t hash = seed;
  for (size_t i = 0; i < size; i++) {
    hash ^= bytes[i];
    hash *= 1099511628211ull;
  }
  return hash;
}

////////////////////////////////////////////////////////////////

// Deflates bytes when zlib is available. Returns false when compression is
// unavailable or did not help, leaving the caller to store the payload as is.
static bool
dbCompress(const std::vector<uint8_t> &raw,
           std::vector<uint8_t> &compressed)
{
#ifdef ZLIB_FOUND
  if (raw.empty())
    return false;
  uLongf bound = compressBound(static_cast<uLong>(raw.size()));
  compressed.resize(bound);
  int status = compress2(compressed.data(), &bound, raw.data(),
                         static_cast<uLong>(raw.size()), Z_BEST_SPEED);
  if (status != Z_OK || bound >= raw.size()) {
    compressed.clear();
    return false;
  }
  compressed.resize(bound);
  return true;
#else
  (void) raw;
  (void) compressed;
  return false;
#endif
}

static void
dbUncompress(const uint8_t *stored,
             size_t stored_size,
             size_t raw_size,
             std::vector<uint8_t> &raw)
{
#ifdef ZLIB_FOUND
  // Deflate cannot expand by more than 1032:1, so a larger claim is a corrupt
  // or crafted header rather than a real payload. Without this the declared
  // size alone decides the allocation, and a few hundred bytes on disk can ask
  // for every byte of address space.
  if (raw_size > stored_size * stadb_max_inflate_ratio)
    throw DbCorrupt("stadb section claims more decompressed bytes than deflate "
                    "can produce");
  raw.resize(raw_size);
  uLongf out_size = static_cast<uLongf>(raw_size);
  int status = uncompress(raw.data(), &out_size, stored,
                          static_cast<uLong>(stored_size));
  if (status != Z_OK || out_size != raw_size)
    throw DbCorrupt("stadb section failed to decompress");
#else
  (void) stored;
  (void) stored_size;
  (void) raw_size;
  (void) raw;
  throw DbCorrupt("stadb file is compressed but zlib is not available");
#endif
}

////////////////////////////////////////////////////////////////

DbFileWriter::DbFileWriter() :
  input_digest_(0)
{
}

void
DbFileWriter::addSection(DbSectionId id, std::vector<uint8_t> bytes)
{
  sections_.emplace_back(id, std::move(bytes));
}

void
DbFileWriter::write(std::string_view filename, bool compress)
{
  // The string table is emitted last because encoding the other sections is
  // what populates it.
  DbWriter string_writer(&strings_);
  const std::vector<std::string> &strings = strings_.strings();
  string_writer.putU64(strings.size());
  for (const std::string &str : strings)
    string_writer.putBlob(str.data(), str.size());
  addSection(DbSectionId::strings, string_writer.takeBytes());

  // Compress payloads first so the table of contents can carry final offsets.
  std::vector<std::vector<uint8_t>> stored(sections_.size());
  std::vector<uint64_t> raw_sizes(sections_.size());
  uint32_t flags = 0;
  for (size_t i = 0; i < sections_.size(); i++) {
    const std::vector<uint8_t> &raw = sections_[i].second;
    raw_sizes[i] = raw.size();
    std::vector<uint8_t> deflated;
    if (compress && dbCompress(raw, deflated)) {
      stored[i] = std::move(deflated);
      flags |= stadb_flag_compressed;
    }
    else
      stored[i] = raw;
  }
  // A section is stored verbatim when deflating it did not pay off, so the
  // per section raw and stored sizes are what actually select the decode path.

  DbWriter header;
  header.putRawU32(stadb_magic);
  header.putRawU32(stadb_version);
  header.putRawU64(stadbAbiGuard());
  header.putRawU32(flags);
  header.putRawU64(input_digest_);
  header.putRawU32(stadb_scene_count);
  header.putRawU32(static_cast<uint32_t>(sections_.size()));

  uint64_t offset = header.size() + sections_.size() * stadb_toc_entry_size;
  for (size_t i = 0; i < sections_.size(); i++) {
    header.putRawU32(static_cast<uint32_t>(sections_[i].first));
    header.putRawU64(offset);
    header.putRawU64(stored[i].size());
    header.putRawU64(raw_sizes[i]);
    header.putRawU32(dbChecksum(stored[i].data(), stored[i].size()));
    offset += stored[i].size();
  }

  std::string name(filename);
  std::ofstream stream(name, std::ios::binary | std::ios::trunc);
  if (!stream.is_open())
    throw FileNotWritable(filename);
  stream.write(reinterpret_cast<const char*>(header.bytes().data()),
               header.size());
  for (const std::vector<uint8_t> &bytes : stored)
    stream.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  stream.close();
  if (stream.fail())
    throw FileNotWritable(filename);
}

////////////////////////////////////////////////////////////////

DbFileReader::DbFileReader() :
  input_digest_(0)
{
}

void
DbFileReader::read(std::string_view filename)
{
  std::string name(filename);
  std::ifstream stream(name, std::ios::binary | std::ios::ate);
  if (!stream.is_open())
    throw FileNotReadable(filename);
  std::streamsize size = stream.tellg();
  if (size < 0)
    throw FileNotReadable(filename);
  // Reject before the file vector is allocated. The section-size cap only
  // runs after the whole file is already in memory.
  if (static_cast<uint64_t>(size) > stadb_max_file_bytes)
    throw DbCorrupt("stadb file is larger than this build will restore");
  stream.seekg(0);
  std::vector<uint8_t> file(static_cast<size_t>(size));
  stream.read(reinterpret_cast<char*>(file.data()), size);
  if (stream.fail())
    throw FileNotReadable(filename);

  DbReader header(file.data(), file.size());
  if (header.getRawU32() != stadb_magic)
    throw DbCorrupt(sta::format("{} is not a stadb file", filename));
  uint32_t version = header.getRawU32();
  if (version != stadb_version)
    throw DbCorrupt(sta::format("stadb version {} but this build writes {}",
                                version, stadb_version));
  uint64_t abi_guard = header.getRawU64();
  if (abi_guard != stadbAbiGuard())
    throw DbCorrupt("stadb was written by a build with different data "
                    "structure layouts");
  header.getRawU32();
  input_digest_ = header.getRawU64();
  uint32_t scene_count = header.getRawU32();
  if (scene_count != stadb_scene_count)
    throw DbCorrupt(sta::format("stadb has {} scenes but this build supports "
                                "only {}", scene_count, stadb_scene_count));
  uint32_t section_count = header.getRawU32();

  for (uint32_t i = 0; i < section_count; i++) {
    DbSectionId id = static_cast<DbSectionId>(header.getRawU32());
    uint64_t offset = header.getRawU64();
    uint64_t stored_size = header.getRawU64();
    uint64_t raw_size = header.getRawU64();
    uint32_t checksum = header.getRawU32();
    // Subtraction, because offset + stored_size can wrap and describe a range
    // that looks like it fits while pointing outside the file.
    if (offset > file.size() || stored_size > file.size() - offset)
      throw DbCorrupt("stadb section extends past end of file");
    const uint8_t *stored = file.data() + offset;
    if (dbChecksum(stored, stored_size) != checksum)
      throw DbCorrupt("stadb section checksum mismatch");
    // Ratio-valid headers can still name a size that will not fit in memory.
    // Reject those before the section vector is allocated.
    if (raw_size > stadb_max_section_bytes)
      throw DbCorrupt("stadb section claims more bytes than this build will "
                      "restore");
    std::vector<uint8_t> raw;
    if (stored_size == raw_size)
      raw.assign(stored, stored + stored_size);
    else
      dbUncompress(stored, stored_size, raw_size, raw);
    sections_[id] = std::move(raw);
  }

  if (!hasSection(DbSectionId::strings))
    throw DbCorrupt("stadb has no string table");
  DbReader string_reader = sectionReader(DbSectionId::strings);
  size_t string_count = string_reader.getCount("string table");
  std::vector<std::string> strings;
  strings.reserve(string_count);
  for (size_t i = 0; i < string_count; i++) {
    std::string_view str = string_reader.getBlob();
    strings.emplace_back(str);
  }
  string_reader.checkFullyConsumed("string table");
  strings_.setStrings(std::move(strings));
}

bool
DbFileReader::hasSection(DbSectionId id) const
{
  return sections_.find(id) != sections_.end();
}

DbReader
DbFileReader::sectionReader(DbSectionId id)
{
  auto itr = sections_.find(id);
  if (itr == sections_.end())
    throw DbCorrupt("stadb is missing a required section");
  return DbReader(itr->second.data(), itr->second.size(), &strings_);
}

} // namespace sta
