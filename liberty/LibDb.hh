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
//
// Compiles the timing-relevant subset of a liberty library into a binary form
// that reloads without running the liberty parser. Reconstruction goes through
// LibertyBuilder and the public setters, so a loaded library is an ordinary
// LibertyLibrary with no special-cased behaviour downstream.
//
// Scope: NLDM timing only. CCS, LVF/POCV and power groups are intentionally
// not stored; see kFlagHasCcs/kFlagHasLvf below.

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

// Bumped whenever the on-disk layout changes. Readers reject any other value
// rather than trying to interpret an older layout.
constexpr uint32_t kLibDbVersion = 1;
constexpr char kLibDbMagic[8] = {'S', 'T', 'A', 'L', 'I', 'B', 'D', 'B'};

// Capability bits. All are 0 today; they exist so a future writer can add CCS
// or LVF without a version bump, and so the engine can refuse delay
// calculators that would silently fall back to NLDM.
constexpr uint32_t kFlagHasCcs = 1u << 0;
constexpr uint32_t kFlagHasLvf = 1u << 1;
constexpr uint32_t kFlagHasPower = 1u << 2;

struct LibDbHeader
{
  char magic[8];
  uint32_t version;
  uint32_t flags;
  // infer_latches changes which timing arcs finish() creates, so a db compiled
  // with it set is not interchangeable with one compiled without.
  uint32_t infer_latches;
  uint32_t pointer_size;
  uint64_t string_bytes;
  uint64_t body_bytes;
};

// Serialization uses native byte order and native float layout; the header
// records pointer size and the reader validates it. Databases are portable
// between machines of the same architecture, which is the stated scope.

class DbWriter
{
public:
  void u8(uint8_t v) { raw(&v, sizeof v); }
  void u32(uint32_t v) { raw(&v, sizeof v); }
  void u64(uint64_t v) { raw(&v, sizeof v); }
  void i32(int32_t v) { raw(&v, sizeof v); }
  void f32(float v) { raw(&v, sizeof v); }
  void boolean(bool v) { u8(v ? 1 : 0); }

  void floats(const std::vector<float> &v)
  {
    u32(static_cast<uint32_t>(v.size()));
    if (!v.empty())
      raw(v.data(), v.size() * sizeof(float));
  }

  // Strings are stored once in a side table and referenced by index, which
  // matters because cell and port names repeat heavily across a library.
  void str(std::string_view s) { u32(internString(s)); }

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

class DbReader
{
public:
  DbReader(const uint8_t *data,
           size_t size,
           const std::vector<std::string> *strings) :
    data_(data),
    size_(size),
    strings_(strings)
  {
  }

  uint8_t u8() { return read<uint8_t>(); }
  uint32_t u32() { return read<uint32_t>(); }
  uint64_t u64() { return read<uint64_t>(); }
  int32_t i32() { return read<int32_t>(); }
  float f32() { return read<float>(); }
  bool boolean() { return u8() != 0; }

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

  bool ok(size_t n)
  {
    if (pos_ + n > size_) {
      fail();
      return false;
    }
    return true;
  }

  void fail() { failed_ = true; }

  const uint8_t *data_;
  size_t size_;
  size_t pos_{0};
  const std::vector<std::string> *strings_;
  bool failed_{false};
};

// Compile an already loaded liberty library to filename.
void writeLibDbFile(LibertyLibrary *library,
                    std::string_view filename,
                    Report *report);

// Rebuild a liberty library from filename and register it with network.
LibertyLibrary *readLibDbFile(std::string_view filename,
                              Network *network);

} // namespace sta
