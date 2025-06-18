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

#pragma once

#include <memory>

#include "Vector.hh"
#include "Transition.hh"
#include "Delay.hh"
#include "LibertyClass.hh"

namespace sta {

class TimingArcAttrs;
class WireTimingArc;
class GateTableModel;
class DcalcAnalysisPt;

typedef int TimingArcIndex;
typedef Vector<TimingArc*> TimingArcSeq;
typedef Map<const OperatingConditions*, TimingModel*> ScaledTimingModelMap;

enum class TimingType {
  clear,
  combinational,
  combinational_fall,
  combinational_rise,
  falling_edge,
  hold_falling,
  hold_rising,
  min_pulse_width,
  minimum_period,
  nochange_high_high,
  nochange_high_low,
  nochange_low_high,
  nochange_low_low,
  non_seq_hold_falling,
  non_seq_hold_rising,
  non_seq_setup_falling,
  non_seq_setup_rising,
  preset,
  recovery_falling,
  recovery_rising,
  removal_falling,
  removal_rising,
  retaining_time,
  rising_edge,
  setup_falling,
  setup_rising,
  skew_falling,
  skew_rising,
  three_state_disable,
  three_state_disable_fall,
  three_state_disable_rise,
  three_state_enable,
  three_state_enable_fall,
  three_state_enable_rise,
  min_clock_tree_path,
  max_clock_tree_path,
  unknown
};

const char *
to_string(TimingType type);
TimingType
findTimingType(const char *string);
bool
timingTypeIsCheck(TimingType type);
ScaleFactorType
timingTypeScaleFactorType(TimingType type);

////////////////////////////////////////////////////////////////

class TimingArcAttrs
{
public:
  TimingArcAttrs();
  TimingArcAttrs(TimingSense sense);
  virtual ~TimingArcAttrs();
  TimingType timingType() const { return timing_type_; }
  void setTimingType(TimingType type);
  TimingSense timingSense() const { return timing_sense_; }
  void setTimingSense(TimingSense sense);
  FuncExpr *cond() const { return cond_; }
  void setCond(FuncExpr *cond);
  const char *sdfCond() const { return sdf_cond_; }
  void setSdfCond(const char *cond);
  const char *sdfCondStart() const { return sdf_cond_start_; }
  void setSdfCondStart(const char *cond);
  const char *sdfCondEnd() const { return sdf_cond_end_; }
  void setSdfCondEnd(const char *cond);
  const char *modeName() const { return mode_name_; }
  void setModeName(const char *name);
  const char *modeValue() const { return mode_value_; }
  void setModeValue(const char *value);
  TimingModel *model(const RiseFall *rf) const;
  void setModel(const RiseFall *rf,
		TimingModel *model);
  float ocvArcDepth() const { return ocv_arc_depth_; }
  void setOcvArcDepth(float depth);

protected:
  TimingType timing_type_;
  TimingSense timing_sense_;
  FuncExpr *cond_;
  const char *sdf_cond_;
  const char *sdf_cond_start_;
  const char *sdf_cond_end_;
  const char *mode_name_;
  const char *mode_value_;
  float ocv_arc_depth_;
  TimingModel *models_[RiseFall::index_count];
};

// A timing arc set is a group of related timing arcs between from/to
// a pair of cell ports.  Wire timing arcs are a special set owned by
// the TimingArcSet class.
//
// See ~LibertyCell for delete of TimingArcSet members.
class TimingArcSet
{
public:
  TimingArcSet(LibertyCell *cell,
	       LibertyPort *from,
	       LibertyPort *to,
	       LibertyPort *related_out,
	       const TimingRole *role,
	       TimingArcAttrsPtr attrs);
  virtual ~TimingArcSet();
  LibertyCell *libertyCell() const;
  LibertyPort *from() const { return from_; }
  LibertyPort *to() const { return to_; }
  bool isWire() const;
  LibertyPort *relatedOut() const { return related_out_; }
  const TimingRole *role() const { return role_; };
  TimingSense sense() const;
  // Rise/fall if the arc set is rising_edge or falling_edge.
  const RiseFall *isRisingFallingEdge() const;
  size_t arcCount() const { return arcs_.size(); }
  TimingArcSeq &arcs() { return arcs_; }
  // Return 1 or 2 arcs matching from transition.
  void arcsFrom(const RiseFall *from_rf,
		// Return values.
		TimingArc *&arc1,
		TimingArc *&arc2) const;
  TimingArc *arcTo(const RiseFall *to_rf) const;
  const TimingArcSeq &arcs() const { return arcs_; }
  TimingArcIndex addTimingArc(TimingArc *arc);
  void deleteTimingArc(TimingArc *arc);
  TimingArc *findTimingArc(unsigned arc_index);
  void setRole(const TimingRole *role);
  FuncExpr *cond() const { return attrs_->cond(); }
  // Cond default is the timing arcs with no condition when there are
  // other conditional timing arcs between the same pins.
  bool isCondDefault() const { return is_cond_default_; }
  void setIsCondDefault(bool is_default);
  // SDF IOPATHs match sdfCond.
  // sdfCond (IOPATH) reuses sdfCondStart (timing check) variable.
  const char *sdfCond() const { return attrs_->sdfCondStart(); }
  // SDF timing checks match sdfCondStart/sdfCondEnd.
  const char *sdfCondStart() const { return attrs_->sdfCondStart(); }
  const char *sdfCondEnd() const { return attrs_->sdfCondEnd(); }
  const char *modeName() const { return attrs_->modeName(); }
  const char *modeValue() const { return attrs_->modeValue(); }
  // Timing arc set index in cell.
  TimingArcIndex index() const { return index_; }
  bool isDisabledConstraint() const { return is_disabled_constraint_; }
  void setIsDisabledConstraint(bool is_disabled);
  // OCV arc depth from timing/cell/library.
  float ocvArcDepth() const;

  static bool equiv(const TimingArcSet *set1,
		    const TimingArcSet *set2);
  static bool less(const TimingArcSet *set1,
		   const TimingArcSet *set2);

  static void init();
  static void destroy();

  // Psuedo definition for wire arcs.
  static TimingArcSet *wireTimingArcSet() { return wire_timing_arc_set_; }
  static int wireArcIndex(const RiseFall *rf);
  static int wireArcCount() { return 2; }

protected:
  TimingArcSet(const TimingRole *role,
               TimingArcAttrsPtr attrs);

  LibertyPort *from_;
  LibertyPort *to_;
  LibertyPort *related_out_;
  const TimingRole *role_;
  // TimingArcAttrs are shared by TimingArcSets in a bus with timing groups. 
  TimingArcAttrsPtr attrs_;
  TimingArcSeq arcs_;
  bool is_cond_default_;
  unsigned index_;
  bool is_disabled_constraint_;
  TimingArc *from_arc1_[RiseFall::index_count];
  TimingArc *from_arc2_[RiseFall::index_count];
  TimingArc *to_arc_[RiseFall::index_count];

  static TimingArcAttrsPtr wire_timing_arc_attrs_;
  static TimingArcSet *wire_timing_arc_set_;
};

// A timing arc is a single from/to transition between two ports.
// The timing model parameters used for delay calculation are also found here.
class TimingArc
{
public:
  TimingArc(TimingArcSet *set,
	    const Transition *from_rf,
	    const Transition *to_rf,
	    TimingModel *model);
  ~TimingArc();
  std::string to_string() const;
  LibertyPort *from() const { return set_->from(); }
  LibertyPort *to() const { return set_->to(); }
  const Transition *fromEdge() const { return from_rf_; }
  const Transition *toEdge() const { return to_rf_; }
  const TimingRole *role() const { return set_->role(); }
  TimingArcSet *set() const { return set_; }
  TimingSense sense() const;
  // Index in TimingArcSet.
  unsigned index() const { return index_; }
  TimingModel *model() const { return model_; }
  GateTimingModel *gateModel(const DcalcAnalysisPt *dcalc_ap) const;
  CheckTimingModel *checkModel(const DcalcAnalysisPt *dcalc_ap) const;
  GateTableModel *gateTableModel() const;
  GateTableModel *gateTableModel(const DcalcAnalysisPt *dcalc_ap) const;
  const TimingArc *cornerArc(int ap_index) const;
  void setCornerArc(TimingArc *corner_arc,
		    int ap_index);
  float driveResistance() const;
  ArcDelay intrinsicDelay() const;

  static bool equiv(const TimingArc *arc1,
		    const TimingArc *arc2);

protected:
  TimingModel *model(const DcalcAnalysisPt *dcalc_ap) const;
  void setIndex(unsigned index);
  void addScaledModel(const OperatingConditions *op_cond,
		      TimingModel *scaled_model);

  TimingArcSet *set_;
  const Transition *from_rf_;
  const Transition *to_rf_;
  unsigned index_;
  TimingModel *model_;
  ScaledTimingModelMap *scaled_models_;
  Vector<TimingArc*> corner_arcs_;

private:
  friend class LibertyLibrary;
  friend class LibertyCell;
  friend class TimingArcSet;
};

} // namespace
