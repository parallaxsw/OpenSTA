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

#include "DbCodec.hh"

#include "Format.hh"

namespace sta {

DbCorrupt::DbCorrupt(const std::string &msg) :
  msg_(sta::format("{} {}", stadb_error_corrupt, msg))
{
}

const char *
DbCorrupt::what() const noexcept
{
  return msg_.c_str();
}

////////////////////////////////////////////////////////////////

DbStringTable::DbStringTable()
{
  clear();
}

void
DbStringTable::clear()
{
  strings_.clear();
  ids_.clear();
  // Id 0 is the null/empty string.
  strings_.emplace_back("");
  ids_.emplace("", 0);
}

DbStrId
DbStringTable::intern(std::string_view str)
{
  std::string key(str);
  auto itr = ids_.find(key);
  if (itr != ids_.end())
    return DbStrId(itr->second);
  uint32_t id = static_cast<uint32_t>(strings_.size());
  strings_.push_back(key);
  ids_.emplace(std::move(key), id);
  return DbStrId(id);
}

std::string_view
DbStringTable::string(DbStrId id) const
{
  if (id.id < strings_.size())
    return strings_[id.id];
  return std::string_view();
}

const char *
DbStringTable::cstring(DbStrId id) const
{
  if (id.isNull() || id.id >= strings_.size())
    return nullptr;
  return strings_[id.id].c_str();
}

void
DbStringTable::setStrings(std::vector<std::string> strings)
{
  strings_ = std::move(strings);
  ids_.clear();
  for (uint32_t id = 0; id < strings_.size(); id++)
    ids_.emplace(strings_[id], id);
}

////////////////////////////////////////////////////////////////

DbWriter::DbWriter(DbStringTable *strings) :
  strings_(strings)
{
}

void
DbWriter::putU8(uint8_t v)
{
  bytes_.push_back(v);
}

void
DbWriter::putU64(uint64_t v)
{
  while (v >= 0x80) {
    bytes_.push_back(static_cast<uint8_t>(v) | 0x80);
    v >>= 7;
  }
  bytes_.push_back(static_cast<uint8_t>(v));
}

void
DbWriter::putI64(int64_t v)
{
  // Zigzag so that small negatives stay short.
  putU64((static_cast<uint64_t>(v) << 1) ^ static_cast<uint64_t>(v >> 63));
}

void
DbWriter::putF32(float v)
{
  putBytes(&v, sizeof(v));
}

void
DbWriter::putF64(double v)
{
  putBytes(&v, sizeof(v));
}

void
DbWriter::putStr(std::string_view str)
{
  if (strings_ == nullptr)
    throw DbCorrupt("stadb writer has no string table");
  putU32(strings_->intern(str).id);
}

void
DbWriter::putBytes(const void *data, size_t size)
{
  const uint8_t *bytes = static_cast<const uint8_t*>(data);
  bytes_.insert(bytes_.end(), bytes, bytes + size);
}

void
DbWriter::putBlob(const void *data, size_t size)
{
  putU64(size);
  putBytes(data, size);
}

////////////////////////////////////////////////////////////////

DbReader::DbReader(const uint8_t *data,
                   size_t size,
                   DbStringTable *strings) :
  data_(data),
  size_(size),
  pos_(0),
  strings_(strings)
{
}

void
DbReader::require(size_t count) const
{
  // Subtraction, because a corrupt length can be large enough that pos_ + count
  // wraps and lands back inside the section.
  if (count > size_ - pos_)
    throw DbCorrupt(sta::format("stadb section truncated at offset {}", pos_));
}

size_t
DbReader::getCount(const char *what)
{
  uint64_t count = getU64();
  // Every element costs at least one byte, so a count larger than the bytes
  // left cannot be honest. Checking here means a crafted file is rejected
  // before the count reaches a reserve, instead of after the allocator has
  // already tried to find room for it.
  if (count > remaining())
    throw DbCorrupt(sta::format("stadb {} count {} exceeds the {} bytes left "
                                "in the section", what, count, remaining()));
  return static_cast<size_t>(count);
}

uint8_t
DbReader::getU8()
{
  require(1);
  return data_[pos_++];
}

uint64_t
DbReader::getU64()
{
  uint64_t result = 0;
  int shift = 0;
  while (true) {
    uint8_t byte = getU8();
    result |= static_cast<uint64_t>(byte & 0x7f) << shift;
    if ((byte & 0x80) == 0)
      break;
    shift += 7;
    if (shift >= 64)
      throw DbCorrupt("stadb varint overflow");
  }
  return result;
}

uint16_t
DbReader::getU16()
{
  return static_cast<uint16_t>(getU64());
}

uint32_t
DbReader::getU32()
{
  return static_cast<uint32_t>(getU64());
}

int64_t
DbReader::getI64()
{
  uint64_t encoded = getU64();
  return static_cast<int64_t>(encoded >> 1) ^ -static_cast<int64_t>(encoded & 1);
}

int32_t
DbReader::getI32()
{
  return static_cast<int32_t>(getI64());
}

uint32_t
DbReader::getRawU32()
{
  uint32_t v;
  getBytes(&v, sizeof(v));
  return v;
}

uint64_t
DbReader::getRawU64()
{
  uint64_t v;
  getBytes(&v, sizeof(v));
  return v;
}

float
DbReader::getF32()
{
  float v;
  getBytes(&v, sizeof(v));
  return v;
}

double
DbReader::getF64()
{
  double v;
  getBytes(&v, sizeof(v));
  return v;
}

std::string_view
DbReader::getStr()
{
  if (strings_ == nullptr)
    throw DbCorrupt("stadb reader has no string table");
  return strings_->string(DbStrId(getU32()));
}

const char *
DbReader::getCstring()
{
  if (strings_ == nullptr)
    throw DbCorrupt("stadb reader has no string table");
  return strings_->cstring(DbStrId(getU32()));
}

void
DbReader::getBytes(void *data, size_t size)
{
  require(size);
  std::memcpy(data, data_ + pos_, size);
  pos_ += size;
}

std::string_view
DbReader::getBlob()
{
  size_t size = getU64();
  require(size);
  std::string_view blob(reinterpret_cast<const char*>(data_ + pos_), size);
  pos_ += size;
  return blob;
}

void
DbReader::checkFullyConsumed(const char *what) const
{
  if (pos_ != size_)
    throw DbCorrupt(sta::format("stadb {} section has {} trailing bytes",
                                what, size_ - pos_));
}

} // namespace sta
