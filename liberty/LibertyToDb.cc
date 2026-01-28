// OpenSTA, Static Timing Analyzer
// Copyright (c) 2025, Parallax Software, Inc.
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

#include "LibertyToDb.hh"

#include "LibertyParser.hh"

#ifdef HAVE_CEREAL
#include <fstream>

#include "LibertyDbSerialization.hh"
#endif

class Report;
class Debug;

namespace sta {

#ifdef HAVE_CEREAL
namespace {

SerAttrValue
toSerAttrValue(LibertyAttrValue* v)
{
  SerAttrValue out;
  if (v->isString()) {
    out.is_string = true;
    out.str_val = v->stringValue();
  } else {
    out.is_string = false;
    out.float_val = v->floatValue();
  }
  return out;
}

SerGroup
toSerGroup(LibertyGroup* group)
{
  SerGroup out;
  out.type = group->type();
  out.line = group->line();

  LibertyAttrValueSeq* params = group->params();
  if (params) {
    for (LibertyAttrValue* v : *params)
      out.params.push_back(toSerAttrValue(v));
  }

  LibertyAttrSeq* attrs = group->attrs();
  if (attrs) {
    for (LibertyAttr* attr : *attrs) {
      if (attr->isSimple()) {
        SerSimpleAttr sa;
        sa.name = attr->name();
        sa.line = attr->line();
        sa.value = toSerAttrValue(attr->firstValue());
        out.simple_attrs.push_back(sa);
      } else {
        SerComplexAttr ca;
        ca.name = attr->name();
        ca.line = attr->line();
        LibertyAttrValueSeq* vals = attr->values();
        if (vals) {
          for (LibertyAttrValue* v : *vals)
            ca.values.push_back(toSerAttrValue(v));
        }
        out.complex_attrs.push_back(ca);
      }
    }
  }

  LibertyDefineMap* defs = group->defines();
  if (defs) {
    for (const auto& p : *defs) {
      SerDefine sd;
      sd.name = p.second->name();
      sd.group_type = static_cast<int>(p.second->groupType());
      sd.value_type = static_cast<int>(p.second->valueType());
      sd.line = p.second->line();
      out.defines.push_back(sd);
    }
  }

  LibertyGroupSeq* subs = group->subgroups();
  if (subs) {
    for (LibertyGroup* sub : *subs)
      out.subgroups.push_back(toSerGroup(sub));
  }

  return out;
}

SerVariable
toSerVariable(LibertyVariable* var)
{
  SerVariable out;
  out.var = var->variable();
  out.value = var->value();
  out.line = var->line();
  return out;
}

LibertyAttrValue *
fromSerAttrValue(const SerAttrValue &v)
{
  if (v.is_string)
    return new LibertyStringAttrValue(v.str_val);
  else
    return new LibertyFloatAttrValue(v.float_val);
}

LibertyGroup *
fromSerGroup(const SerGroup &sg)
{
  LibertyAttrValueSeq *params = new LibertyAttrValueSeq;
  for (const auto &p : sg.params)
    params->push_back(fromSerAttrValue(p));

  LibertyGroup *group =
    new LibertyGroup(sg.type, params, sg.line);

  for (const auto &sa : sg.simple_attrs) {
    LibertyAttrValue *val = fromSerAttrValue(sa.value);
    group->addAttribute(new LibertySimpleAttr(sa.name, val, sa.line));
  }

  for (const auto &ca : sg.complex_attrs) {
    LibertyAttrValueSeq *vals = new LibertyAttrValueSeq;
    for (const auto &v : ca.values)
      vals->push_back(fromSerAttrValue(v));
    group->addAttribute(
      new LibertyComplexAttr(ca.name, vals, ca.line));
  }

  for (const auto &sd : sg.defines) {
    group->addDefine(new LibertyDefine(
      sd.name,
      static_cast<LibertyGroupType>(sd.group_type),
      static_cast<LibertyAttrType>(sd.value_type),
      sd.line));
  }

  for (const auto &sub : sg.subgroups)
    group->addSubgroup(fromSerGroup(sub));

  return group;
}

}  // namespace
#endif  // HAVE_CEREAL

class LibertyToDb : public LibertyGroupVisitor
{
public:
  LibertyToDb(Report *report,
              Debug *debug);
  virtual ~LibertyToDb() {}
  void readLiberty(const char *lib_filename);
  void writeDb(const char *dsb_filename);
  bool save(LibertyGroup *) override { return true; }
  bool save(LibertyAttr *) override { return true; }
  bool save(LibertyVariable *) override { return true; }

  void begin(LibertyGroup *group) override;
  void end(LibertyGroup *) override {}
  void visitAttr(LibertyAttr *) override {}
  void visitVariable(LibertyVariable *variable) override;

protected:
  std::string lib_filename_;
  std::string db_filename_;
  LibertyGroup *library_;
#ifdef HAVE_CEREAL
  std::vector<LibertyVariable*> variables_;
#endif
  Report *report_;
  Debug *debug_;
};

void
writeLibertyDb(const char *lib_filename,
               const char *db_filename,
               Report *report,
               Debug *debug)
{
  LibertyToDb lib_to_db(report, debug);
  lib_to_db.readLiberty(lib_filename);
  lib_to_db.writeDb(db_filename);
}

LibertyToDb::LibertyToDb(Report *report,
                         Debug *debug) :
  library_(nullptr),
  report_(report),
  debug_(debug)
{
}

void
LibertyToDb::visitVariable(LibertyVariable *variable)
{
#ifdef HAVE_CEREAL
  variables_.push_back(variable);
#else
  (void) variable;
#endif
}

void
LibertyToDb::readLiberty(const char *lib_filename)
{
  lib_filename_ = lib_filename;
  parseLibertyFile(lib_filename, this, report_);
}

void
LibertyToDb::writeDb(const char *db_filename)
{
  db_filename_ = db_filename;
#ifdef HAVE_CEREAL
  if (!library_)
    return;

  LibertyDbRoot root;
  root.library = toSerGroup(library_);
  for (LibertyVariable* var : variables_)
    root.variables.push_back(toSerVariable(var));

  std::ofstream ofs(db_filename, std::ios::binary);
  if (!ofs.is_open())
    return;
  cereal::BinaryOutputArchive ar(ofs);
  ar(root);
#endif
}

void
LibertyToDb::begin(LibertyGroup *group)
{
  if (group->type() == "library")
    library_ = group;
}

#ifdef HAVE_CEREAL
void
readLibertyDb(const char *db_filename)
{
  std::ifstream ifs(db_filename, std::ios::binary);
  if (!ifs.is_open())
    return;
  cereal::BinaryInputArchive ar(ifs);
  LibertyDbRoot root;
  ar(root);
  LibertyGroup *library = fromSerGroup(root.library);
  (void) library;  // local only; use in separate request
}
#else
void
readLibertyDb(const char *db_filename)
{
  (void) db_filename;
}
#endif

} // namespace
