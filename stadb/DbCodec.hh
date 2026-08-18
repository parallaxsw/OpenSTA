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

#pragma once

#include <bit>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Error.hh"

namespace sta {

// Report::errorMsg stores "id message"; the Tcl wrapper prints "Error: " +
// what(). 2740 is the Tcl unreadable-file check; 2741 is the CCS drop warning.
constexpr int stadb_error_corrupt = 2742;
constexpr int stadb_error_unsupported = 2743;

// Floats are stored as raw native bytes, so the format is little endian only.
static_assert(std::endian::native == std::endian::little,
              "stadb requires a little endian host");

// Thrown when a .stadb file is truncated, malformed, or written by an
// incompatible build. Callers treat this as a cache miss and regenerate from
// source rather than failing the run.
class DbCorrupt : public Exception
{
public:
  explicit DbCorrupt(const std::string &msg);
  const char *what() const noexcept override;

private:
  std::string msg_;
};

// Enum singletons and pooled objects come back null for an index or name the
// file made up. Every lookup driven by file data goes through here, because the
// result is used as an object and a bad id is indistinguishable from a good one
// until something dereferences it.
template <class T>
inline T *
dbCheck(T *object,
        const char *what)
{
  if (object == nullptr)
    throw DbCorrupt(std::string("stadb ") + what + " could not be resolved");
  return object;
}

// A string table id. Distinct from a bare uint32_t so that assigning an
// interned id to a narrower record field fails to compile. Such a truncation
// is invisible until the table grows past the field's range, at which point it
// silently resolves to the wrong string.
struct DbStrId
{
  uint32_t id = 0;

  DbStrId() = default;
  explicit DbStrId(uint32_t id) : id(id) {}
  bool isNull() const { return id == 0; }
};

// Interned strings. Id 0 is reserved for the null/empty string so that record
// fields can use 0 as "absent" without a separate presence flag.
class DbStringTable
{
public:
  DbStringTable();
  DbStrId intern(std::string_view str);
  // Empty string for id 0 or any out of range id.
  std::string_view string(DbStrId id) const;
  // Null for id 0, so results can feed APIs that take const char *.
  const char *cstring(DbStrId id) const;
  size_t size() const { return strings_.size(); }
  const std::vector<std::string> &strings() const { return strings_; }
  void clear();
  void setStrings(std::vector<std::string> strings);

private:
  // The map owns its keys rather than viewing into strings_, whose elements
  // move when the vector grows.
  std::vector<std::string> strings_;
  std::unordered_map<std::string, uint32_t> ids_;
};

// Append-only byte buffer. Unsigned integers use LEB128 varints and signed
// integers use zigzag varints, which keeps the dense object id fields that
// dominate the graph sections down to one or two bytes each.
//
// The single-argument reference form of each accessor is the half of the
// symmetric codec used by the generated record visitors; DbReader declares the
// same names so one visitor function serves both directions.
class DbWriter
{
public:
  explicit DbWriter(DbStringTable *strings = nullptr);

  // Symmetric visitor API. Mirrored by DbReader.
  void u8(uint8_t &v) { putU8(v); }
  void u16(uint16_t &v) { putU16(v); }
  void u32(uint32_t &v) { putU32(v); }
  void u64(uint64_t &v) { putU64(v); }
  void i32(int32_t &v) { putI32(v); }
  void i64(int64_t &v) { putI64(v); }
  void f32(float &v) { putF32(v); }
  void f64(double &v) { putF64(v); }
  void b(bool &v) { putBool(v); }
  void sid(DbStrId &v) { putU32(v.id); }

  void putU8(uint8_t v);
  void putU16(uint16_t v) { putU64(v); }
  void putU32(uint32_t v) { putU64(v); }
  void putU64(uint64_t v);
  void putI32(int32_t v) { putI64(v); }
  void putI64(int64_t v);
  void putF32(float v);
  void putF64(double v);
  void putBool(bool v) { putU8(v ? 1 : 0); }
  // Fixed width little endian, for the file header and section table whose
  // fields must sit at computable offsets.
  void putRawU32(uint32_t v) { putBytes(&v, sizeof(v)); }
  void putRawU64(uint64_t v) { putBytes(&v, sizeof(v)); }
  // Interns str and writes its id. Requires a string table.
  void putStr(std::string_view str);
  void putBytes(const void *data, size_t size);
  // Length prefixed raw bytes, for payloads with their own encoding.
  void putBlob(const void *data, size_t size);

  DbStringTable *strings() const { return strings_; }
  const std::vector<uint8_t> &bytes() const { return bytes_; }
  std::vector<uint8_t> takeBytes() { return std::move(bytes_); }
  size_t size() const { return bytes_.size(); }
  void clear() { bytes_.clear(); }

private:
  std::vector<uint8_t> bytes_;
  DbStringTable *strings_;
};

// Bounds checked reader over a byte range owned by the caller.
class DbReader
{
public:
  DbReader(const uint8_t *data,
           size_t size,
           DbStringTable *strings = nullptr);

  // Symmetric visitor API. Mirrored by DbWriter.
  void u8(uint8_t &v) { v = getU8(); }
  void u16(uint16_t &v) { v = getU16(); }
  void u32(uint32_t &v) { v = getU32(); }
  void u64(uint64_t &v) { v = getU64(); }
  void i32(int32_t &v) { v = getI32(); }
  void i64(int64_t &v) { v = getI64(); }
  void f32(float &v) { v = getF32(); }
  void f64(double &v) { v = getF64(); }
  void b(bool &v) { v = getBool(); }
  void sid(DbStrId &v) { v = DbStrId(getU32()); }

  uint8_t getU8();
  uint16_t getU16();
  uint32_t getU32();
  uint64_t getU64();
  int32_t getI32();
  int64_t getI64();
  float getF32();
  double getF64();
  bool getBool() { return getU8() != 0; }
  uint32_t getRawU32();
  uint64_t getRawU64();
  // Reads a string id and resolves it. Requires a string table.
  std::string_view getStr();
  const char *getCstring();
  void getBytes(void *data, size_t size);
  // Returns a view into the underlying buffer; valid while the buffer lives.
  std::string_view getBlob();
  // Reads an element count that is about to size a container. Throws unless the
  // section still holds a byte per element, so a small corrupt file cannot ask
  // the reader to reserve more memory than the file could possibly describe.
  size_t getCount(const char *what);

  DbStringTable *strings() const { return strings_; }
  size_t offset() const { return pos_; }
  size_t remaining() const { return size_ - pos_; }
  bool atEnd() const { return pos_ >= size_; }
  // Throws unless the section was consumed exactly, which catches a writer and
  // reader that disagree about a record layout.
  void checkFullyConsumed(const char *what) const;

private:
  void require(size_t count) const;

  const uint8_t *data_;
  size_t size_;
  size_t pos_;
  DbStringTable *strings_;
};

// Record definition helpers.
//
// A record is declared once as an X-macro field list and expanded into both a
// POD struct and a single visitor template. Because the writer and the reader
// share that one visitor, they cannot drift apart or disagree about field
// order, which is the failure mode this format is most exposed to.
//
//   #define STADB_REC_FOO(X) \
//     X(u32, id)             \
//     X(f32, value)
//   STADB_RECORD(FooRec, STADB_REC_FOO)
//
// yields struct FooRec with those fields plus visit(Codec &, FooRec &).

#define STADB_TYPE_u8 uint8_t
#define STADB_TYPE_u16 uint16_t
#define STADB_TYPE_u32 uint32_t
#define STADB_TYPE_u64 uint64_t
#define STADB_TYPE_i32 int32_t
#define STADB_TYPE_i64 int64_t
#define STADB_TYPE_f32 float
#define STADB_TYPE_f64 double
#define STADB_TYPE_b bool
#define STADB_TYPE_sid DbStrId

#define STADB_DECL_FIELD(kind, name) STADB_TYPE_##kind name{};
#define STADB_VISIT_FIELD(kind, name) codec.kind(rec.name);

#define STADB_RECORD(rec_name, field_list)              \
  struct rec_name                                       \
  {                                                     \
    field_list(STADB_DECL_FIELD)                        \
  };                                                    \
  template <class Codec>                                \
  inline void visit(Codec &codec, rec_name &rec)        \
  {                                                     \
    field_list(STADB_VISIT_FIELD)                       \
  }

} // namespace sta
