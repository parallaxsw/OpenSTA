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

#include "InputDrive.hh"

namespace sta {

InputDrive::InputDrive()
{
  for (auto rf_index : RiseFall::rangeIndex()) {
    for (auto mm_index : MinMax::rangeIndex())
      drive_cells_[rf_index][mm_index] = nullptr;
  }
}

InputDrive::~InputDrive()
{
  for (auto rf_index : RiseFall::rangeIndex()) {
    for (auto mm_index : MinMax::rangeIndex()) {
      InputDriveCell *drive_cell = drive_cells_[rf_index][mm_index];
      delete drive_cell;
    }
  }
}

void
InputDrive::setSlew(const RiseFallBoth *rf,
		    const MinMaxAll *min_max,
		    float slew)
{
  slews_.setValue(rf, min_max, slew);
}

void
InputDrive::setDriveResistance(const RiseFallBoth *rf,
			       const MinMaxAll *min_max,
			       float res)
{
  drive_resistances_.setValue(rf, min_max, res);
}

void
InputDrive::driveResistance(const RiseFall *rf,
			    const MinMax *min_max,
			    float &res,
			    bool &exists) const
{
  drive_resistances_.value(rf, min_max, res, exists);
}

bool
InputDrive::hasDriveResistance(const RiseFall *rf,
                               const MinMax *min_max) const
{
  return drive_resistances_.hasValue(rf, min_max);
}

bool
InputDrive::driveResistanceMinMaxEqual(const RiseFall *rf) const
{
  float min_res, max_res;
  bool min_exists, max_exists;
  drive_resistances_.value(rf, MinMax::min(), min_res, min_exists);
  drive_resistances_.value(rf, MinMax::max(), max_res, max_exists);
  return min_exists && max_exists && min_res == max_res;
}

void
InputDrive::setDriveCell(const LibertyLibrary *library,
			 const LibertyCell *cell,
			 const LibertyPort *from_port,
			 float *from_slews,
			 const LibertyPort *to_port,
			 const RiseFallBoth *rf,
			 const MinMaxAll *min_max)
{
  for (auto rf_index : rf->rangeIndex()) {
    for (auto mm_index : min_max->rangeIndex()) {
      InputDriveCell *drive = drive_cells_[rf_index][mm_index];
      if (drive) {
	drive->setLibrary(library);
	drive->setCell(cell);
	drive->setFromPort(from_port);
	drive->setFromSlews(from_slews);
	drive->setToPort(to_port);
      }
      else {
	drive = new InputDriveCell(library, cell, from_port,
				   from_slews, to_port);
	drive_cells_[rf_index][mm_index] = drive;
      }
    }
  }
}

void
InputDrive::driveCell(const RiseFall *rf,
		      const MinMax *min_max,
                      // Return values.
		      const LibertyCell *&cell,
		      const LibertyPort *&from_port,
		      float *&from_slews,
		      const LibertyPort *&to_port) const
{
  InputDriveCell *drive = drive_cells_[rf->index()][min_max->index()];
  if (drive) {
    cell = drive->cell();
    from_port = drive->fromPort();
    from_slews = drive->fromSlews();
    to_port = drive->toPort();
  }
  else {
    cell = nullptr;
    from_port = nullptr;
    from_slews = nullptr;
    to_port = nullptr;
  }
}

InputDriveCell *
InputDrive::driveCell(const RiseFall *rf,
		      const MinMax *min_max) const
{
  return drive_cells_[rf->index()][min_max->index()];
}

bool
InputDrive::hasDriveCell(const RiseFall *rf,
			 const MinMax *min_max) const
{
  return drive_cells_[rf->index()][min_max->index()] != nullptr;
}

bool
InputDrive::driveCellsEqual() const
{
  int rise_index = RiseFall::riseIndex();
  int fall_index = RiseFall::fallIndex();
  int min_index = MinMax::minIndex();
  int max_index = MinMax::maxIndex();
  InputDriveCell *drive1 = drive_cells_[rise_index][min_index];
  InputDriveCell *drive2 = drive_cells_[rise_index][max_index];
  InputDriveCell *drive3 = drive_cells_[fall_index][min_index];
  InputDriveCell *drive4 = drive_cells_[fall_index][max_index];
  return drive1->equal(drive2)
    && drive1->equal(drive3)
    && drive1->equal(drive4);
}

void
InputDrive::slew(const RiseFall *rf,
		 const MinMax *min_max,
		 float &slew,
		 bool &exists) const
{
  slews_.value(rf, min_max, slew, exists);
}

////////////////////////////////////////////////////////////////

InputDriveCell::InputDriveCell(const LibertyLibrary *library,
			       const LibertyCell *cell,
			       const LibertyPort *from_port,
			       float *from_slews,
			       const LibertyPort *to_port) :
  library_(library),
  cell_(cell),
  from_port_(from_port),
  to_port_(to_port)
{
  setFromSlews(from_slews);
}

void
InputDriveCell::setLibrary(const LibertyLibrary *library)
{
  library_ = library;
}

void
InputDriveCell::setCell(const LibertyCell *cell)
{
  cell_ = cell;
}

void
InputDriveCell::setFromPort(const LibertyPort *from_port)
{
  from_port_ = from_port;
}

void
InputDriveCell::setToPort(const LibertyPort *to_port)
{
  to_port_ = to_port;
}

void
InputDriveCell::setFromSlews(float *from_slews)
{
  for (auto rf_index : RiseFall::rangeIndex())
    from_slews_[rf_index] = from_slews[rf_index];
}

bool
InputDriveCell::equal(const InputDriveCell *drive) const
{
  int rise_index = RiseFall::riseIndex();
  int fall_index = RiseFall::fallIndex();
  return cell_ == drive->cell_
    && from_port_ == drive->from_port_
    && from_slews_[rise_index] == drive->from_slews_[rise_index]
    && from_slews_[fall_index] == drive->from_slews_[fall_index]
    && to_port_ == drive->to_port_;
}

} // namespace
