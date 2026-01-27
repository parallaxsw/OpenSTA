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

class Report;
class Debug;

namespace sta {

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
  void visitVariable(LibertyVariable *) override {}

protected:
  std::string lib_filename_;
  std::string db_filename_;
  LibertyGroup *library_;
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
LibertyToDb::readLiberty(const char *lib_filename)
{
  lib_filename_ = lib_filename;
  parseLibertyFile(lib_filename, this, report_);
}

void
LibertyToDb::writeDb(const char *db_filename)
{
  db_filename_ = db_filename;
}

void
LibertyToDb::begin(LibertyGroup *group)
{
  if (group->type() == "library")
    library_ = group;
}

} // namespace
