// OpenSTA, Static Timing Analyzer
// Copyright (c) 2025, Parallax Software, Inc.
//
// Serializable DTOs and Cereal archive support for Liberty DB binary format.

#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <string>
#include <vector>

namespace sta {

// Attribute value: string or float (mirrors LibertyAttrValue)
struct SerAttrValue
{
  bool is_string = true;
  std::string str_val;
  float float_val = 0.0f;

  template <class Archive>
  void serialize(Archive& ar)
  {
    ar(is_string, str_val, float_val);
  }
};

// Mirrors LibertySimpleAttr
struct SerSimpleAttr
{
  std::string name;
  int line = 0;
  SerAttrValue value;

  template <class Archive>
  void serialize(Archive& ar)
  {
    ar(name, line, value);
  }
};

// Mirrors LibertyComplexAttr
struct SerComplexAttr
{
  std::string name;
  int line = 0;
  std::vector<SerAttrValue> values;

  template <class Archive>
  void serialize(Archive& ar)
  {
    ar(name, line, values);
  }
};

// Mirrors LibertyDefine (enums as int)
struct SerDefine
{
  std::string name;
  int group_type = 0;   // LibertyGroupType
  int value_type = 0;   // LibertyAttrType
  int line = 0;

  template <class Archive>
  void serialize(Archive& ar)
  {
    ar(name, group_type, value_type, line);
  }
};

// Mirrors LibertyVariable
struct SerVariable
{
  std::string var;
  float value = 0.0f;
  int line = 0;

  template <class Archive>
  void serialize(Archive& ar)
  {
    ar(var, value, line);
  }
};

// Mirrors LibertyGroup (forward decl for recursive type)
struct SerGroup;

// Root and group container
struct SerGroup
{
  std::string type;
  int line = 0;
  std::vector<SerAttrValue> params;
  std::vector<SerSimpleAttr> simple_attrs;
  std::vector<SerComplexAttr> complex_attrs;
  std::vector<SerDefine> defines;
  std::vector<SerGroup> subgroups;

  template <class Archive>
  void serialize(Archive& ar)
  {
    ar(type, line, params, simple_attrs, complex_attrs, defines, subgroups);
  }
};

// Top-level DB: library group + global variables (collected during parse)
struct LibertyDbRoot
{
  SerGroup library;
  std::vector<SerVariable> variables;

  template <class Archive>
  void serialize(Archive& ar)
  {
    ar(library, variables);
  }
};

} // namespace sta
