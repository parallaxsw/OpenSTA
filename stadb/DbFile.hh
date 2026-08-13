// OpenSTA, Static Timing Analyzer
// Copyright (c) 2026, Parallax Software, Inc.
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

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "DbCodec.hh"
#include "DbFormat.hh"

namespace sta {

// Container writer. Section payloads are built independently in memory and
// indexed by a table of contents, so the string table can be emitted last even
// though the reader needs it first.
class DbFileWriter
{
public:
  DbFileWriter();
  DbStringTable *strings() { return &strings_; }
  void addSection(DbSectionId id, std::vector<uint8_t> bytes);
  // Opaque digest of the source files this snapshot was built from. Callers
  // compare it to decide whether a cached file is still valid.
  void setInputDigest(uint64_t digest) { input_digest_ = digest; }
  void write(std::string_view filename, bool compress);
  size_t sectionCount() const { return sections_.size(); }

private:
  DbStringTable strings_;
  std::vector<std::pair<DbSectionId, std::vector<uint8_t>>> sections_;
  uint64_t input_digest_;
};

// Container reader. Throws DbCorrupt for a malformed file, a format version
// mismatch, or an ABI guard mismatch; all three mean regenerate from source.
class DbFileReader
{
public:
  DbFileReader();
  void read(std::string_view filename);
  DbStringTable *strings() { return &strings_; }
  bool hasSection(DbSectionId id) const;
  // Reader over the decompressed section payload, bound to the string table.
  DbReader sectionReader(DbSectionId id);
  uint64_t inputDigest() const { return input_digest_; }

private:
  DbStringTable strings_;
  std::map<DbSectionId, std::vector<uint8_t>> sections_;
  uint64_t input_digest_;
};

// FNV-1a over a byte range. Used for section checksums and for digesting the
// input file list.
uint32_t dbChecksum(const uint8_t *data, size_t size);
uint64_t dbDigest(uint64_t seed, const void *data, size_t size);

} // namespace sta
