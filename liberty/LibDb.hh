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
// Binary liberty database (.libdb).
// Filename must end in .libdb and works across machines of the same architecture.

#pragma once

#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace sta {

class LibertyLibrary;
class Network;
class Report;

// Bumped when the on-disk layout changes; readers reject any other value.
constexpr uint32_t lib_db_version = 1;
// Means "no object here" for shared axes/tables/attrs ids.
constexpr uint32_t lib_db_id_null = 0xFFFFFFFFu;

enum class LibDbModelKind : uint8_t { none = 0, gate = 1, check = 2 };
enum class LibDbPortKind : uint8_t { scalar = 0, bus = 1, bundle = 2 };

// Header for the libdb file.
struct LibDbHeader
{
  uint32_t version;      // version of the libdb format
  uint64_t string_bytes; // bytes of serialized strings
  uint64_t body_bytes;   // bytes of serialized body
};

// Builds the file body as a growing byte list. Also keeps a list of unique
// strings so names are stored once and referenced by a small number.
class LibDbWriter
{
public:
  void u8(uint8_t v) { raw(&v, sizeof v); }
  void u32(uint32_t v) { raw(&v, sizeof v); }
  void u64(uint64_t v) { raw(&v, sizeof v); }
  void i32(int32_t v) { raw(&v, sizeof v); }
  void f32(float v) { raw(&v, sizeof v); }
  void boolean(bool v) { u8(v ? 1 : 0); }

  // Length-prefixed float array: u32 count, then count floats.
  void floats(const std::vector<float> &v)
  {
    u32(static_cast<uint32_t>(v.size()));
    if (!v.empty())
      raw(v.data(), v.size() * sizeof(float));
  }

  // Write a number that points into the unique-string list (not the text).
  void str(std::string_view s) { u32(internString(s)); }

  // If we have not seen this string before, add it and give it a new number.
  // If we have, return the same number again.
  uint32_t internString(std::string_view s)
  {
    auto [it, inserted] = string_ids_.try_emplace(std::string(s),
                                                  static_cast<uint32_t>(strings_.size()));
    if (inserted)
      strings_.push_back(std::string(s));
    return it->second;
  }

  const std::vector<std::string> &strings() const { return strings_; }
  const std::vector<uint8_t> &bytes() const { return buf_; }
  size_t size() const { return buf_.size(); }

private:
  void raw(const void *p, size_t n)
  {
    const uint8_t *b = static_cast<const uint8_t *>(p);
    buf_.insert(buf_.end(), b, b + n);
  }

  std::vector<uint8_t> buf_;
  std::vector<std::string> strings_;
  std::map<std::string, uint32_t, std::less<>> string_ids_;
};

// Reads the body left-to-right, like a bookmark moving through a byte array:
//
//   data_[0 .. size_)  = whole body
//   pos_               = next byte to read (starts at 0, moves forward)
//
// u32()/f32()/... call read<T>(), which:
//   1) ok(sizeof(T)) — enough bytes left? if not, fail() and return 0
//   2) copy those bytes into a value
//   3) pos_ += sizeof(T)
//
// str() reads a u32 id, then looks up strings_[id] (the side string list).
// At the end of the library load, LibLoader checks failed().
class LibDbReader
{
public:
  LibDbReader(const uint8_t *data,
              size_t size,
              const std::vector<std::string> *strings) :
    data_(data),
    size_(size),
    strings_(strings)
  {
  }

  // Each of these advances pos_ by the size of that type.
  uint8_t u8() { return read<uint8_t>(); }
  uint32_t u32() { return read<uint32_t>(); }
  uint64_t u64() { return read<uint64_t>(); }
  int32_t i32() { return read<int32_t>(); }
  float f32() { return read<float>(); }
  bool boolean() { return u8() != 0; }

  // count (u32), then that many floats in a row.
  std::vector<float> floats()
  {
    uint32_t n = u32();
    std::vector<float> v(n);
    if (n && ok(n * sizeof(float))) {
      std::memcpy(v.data(), data_ + pos_, n * sizeof(float));
      pos_ += n * sizeof(float);
    }
    return v;
  }

  // Read string-list index, return that string (or empty if bad id).
  const std::string &str()
  {
    uint32_t id = u32();
    if (id >= strings_->size()) {
      fail();
      static const std::string empty;
      return empty;
    }
    return (*strings_)[id];
  }

  bool failed() const { return failed_; }

private:
  // Copy the next sizeof(T) bytes at pos_ into a T and slide the bookmark.
  template <typename T>
  T read()
  {
    T v{};
    if (ok(sizeof(T))) {
      std::memcpy(&v, data_ + pos_, sizeof(T));
      pos_ += sizeof(T);
    }
    return v;
  }

  // True if n more bytes fit before size_. On failure, mark failed_.
  bool ok(size_t n)
  {
    if (pos_ + n > size_) {
      fail();
      return false;
    }
    return true;
  }

  void fail() { failed_ = true; }

  const uint8_t *data_;   // body start
  size_t size_;           // body length in bytes
  size_t pos_{0};         // bookmark: next unread byte
  const std::vector<std::string> *strings_;  // side string list
  bool failed_{false};    // true if we read past the end or bad string id
};

// Compile an already loaded liberty library to filename.
void writeLibDbFile(LibertyLibrary *library,
                    std::string_view filename,
                    Report *report);

// Rebuild a liberty library from filename and register it with network.
LibertyLibrary *readLibDbFile(std::string_view filename,
                              Network *network);

} // namespace sta
