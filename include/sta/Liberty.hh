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

#include <mutex>
#include <atomic>

#include "MinMax.hh"
#include "RiseFallMinMax.hh"
#include "ConcreteLibrary.hh"
#include "RiseFallValues.hh"
#include "MinMaxValues.hh"
#include "Transition.hh"
#include "Delay.hh"
#include "LibertyClass.hh"

namespace sta {

class WriteTimingModel;
class LibertyCellIterator;
class LibertyCellPortIterator;
class LibertyCellPortBitIterator;
class LibertyCellPgPortIterator;
class LibertyPortMemberIterator;
class ModeValueDef;
class TestCell;
class PatternMatch;
class LatchEnable;
class Report;
class Debug;
class LibertyBuilder;
class LibertyReader;
class OcvDerate;
class TimingArcAttrs;
class InternalPowerAttrs;
class LibertyPgPort;
class StaState;
class Corner;
class Corners;
class DcalcAnalysisPt;
class DriverWaveform;

typedef Map<const char*, TableTemplate*, CharPtrLess> TableTemplateMap;
typedef Vector<TableTemplate*> TableTemplateSeq;
typedef Map<const char*, BusDcl *, CharPtrLess> BusDclMap;
typedef Vector<BusDcl *> BusDclSeq;
typedef Map<const char*, ScaleFactors*, CharPtrLess> ScaleFactorsMap;
typedef Map<const char*, Wireload*, CharPtrLess> WireloadMap;
typedef Map<const char*, WireloadSelection*, CharPtrLess> WireloadSelectionMap;
typedef Map<const char*, OperatingConditions*,
	    CharPtrLess> OperatingConditionsMap;
typedef Map<LibertyPort*, Sequential*> PortToSequentialMap;
typedef Vector<TimingArcSet*> TimingArcSetSeq;
typedef Set<TimingArcSet*, TimingArcSetLess> TimingArcSetMap;
typedef Map<LibertyPortPair, TimingArcSetSeq*,
	    LibertyPortPairLess> LibertyPortPairTimingArcMap;
typedef Vector<InternalPower*> InternalPowerSeq;
typedef Map<const LibertyPort *, InternalPowerSeq> PortInternalPowerSeq;
typedef Vector<LeakagePower*> LeakagePowerSeq;
typedef Map<const LibertyPort*, TimingArcSetSeq*> LibertyPortTimingArcMap;
typedef Map<const OperatingConditions*, LibertyCell*> ScaledCellMap;
typedef Map<const OperatingConditions*, LibertyPort*> ScaledPortMap;
typedef Map<const char *, ModeDef*, CharPtrLess> ModeDefMap;
typedef Map<const char *, ModeValueDef*, CharPtrLess> ModeValueMap;
typedef Map<const TimingArcSet*, LatchEnable*> LatchEnableMap;
typedef Vector<LatchEnable*> LatchEnableSeq;
typedef Map<const char *, OcvDerate*, CharPtrLess> OcvDerateMap;
typedef Vector<InternalPowerAttrs*> InternalPowerAttrsSeq;
typedef Map<std::string, float> SupplyVoltageMap;
typedef Map<std::string, LibertyPgPort*> LibertyPgPortMap;
typedef Map<std::string, DriverWaveform*> DriverWaveformMap;
typedef Vector<DcalcAnalysisPt*> DcalcAnalysisPtSeq;

enum class ClockGateType { none, latch_posedge, latch_negedge, other };

enum class DelayModelType { cmos_linear, cmos_pwl, cmos2, table, polynomial, dcm };

enum class ScanSignalType { enable, enable_inverted, clock, clock_a, clock_b,
                            input, input_inverted, output, output_inverted, none };

enum class ScaleFactorPvt { process, volt, temp, unknown };
constexpr int scale_factor_pvt_count = int(ScaleFactorPvt::unknown) + 1;

enum class TableTemplateType { delay, power, output_current, capacitance, ocv };
constexpr int table_template_type_count = int(TableTemplateType::ocv) + 1;

enum class LevelShifterType { HL, LH, HL_LH };

enum class SwitchCellType { coarse_grain, fine_grain };

////////////////////////////////////////////////////////////////

void
initLiberty();
void
deleteLiberty();

ScaleFactorPvt
findScaleFactorPvt(const char *name);
const char *
scaleFactorPvtName(ScaleFactorPvt pvt);

ScaleFactorType
findScaleFactorType(const char *name);
const char *
scaleFactorTypeName(ScaleFactorType type);
bool
scaleFactorTypeRiseFallSuffix(ScaleFactorType type);
bool
scaleFactorTypeRiseFallPrefix(ScaleFactorType type);
bool
scaleFactorTypeLowHighSuffix(ScaleFactorType type);

// Timing sense as a string.
const char *
to_string(TimingSense sense);

// Opposite timing sense.
TimingSense
timingSenseOpposite(TimingSense sense);

////////////////////////////////////////////////////////////////

class LibertyLibrary : public ConcreteLibrary
{
public:
  LibertyLibrary(const char *name,
		 const char *filename);
  virtual ~LibertyLibrary();
  LibertyCell *findLibertyCell(const char *name) const;
  LibertyCellSeq findLibertyCellsMatching(PatternMatch *pattern);
  // Liberty cells that are buffers.
  LibertyCellSeq *buffers();
  LibertyCellSeq *inverters();

  DelayModelType delayModelType() const { return delay_model_type_; }
  void setDelayModelType(DelayModelType type);
  void addBusDcl(BusDcl *bus_dcl);
  BusDcl *findBusDcl(const char *name) const;
  BusDclSeq busDcls() const;
  void addTableTemplate(TableTemplate *tbl_template,
			TableTemplateType type);
  TableTemplate *findTableTemplate(const char *name,
				   TableTemplateType type);
  TableTemplateSeq tableTemplates() const;
  float nominalProcess() const { return nominal_process_; }
  void setNominalProcess(float process);
  float nominalVoltage() const { return nominal_voltage_; }
  void setNominalVoltage(float voltage);
  float nominalTemperature() const { return nominal_temperature_; }
  void setNominalTemperature(float temperature);

  void setScaleFactors(ScaleFactors *scales);
  // Add named scale factor group.
  void addScaleFactors(ScaleFactors *scales);
  ScaleFactors *findScaleFactors(const char *name);
  ScaleFactors *scaleFactors() const { return scale_factors_; }
  float scaleFactor(ScaleFactorType type,
		    const Pvt *pvt) const;
  float scaleFactor(ScaleFactorType type,
		    const LibertyCell *cell,
		    const Pvt *pvt) const;
  float scaleFactor(ScaleFactorType type,
		    int rf_index,
		    const LibertyCell *cell,
		    const Pvt *pvt) const;

  void setWireSlewDegradationTable(TableModel *model,
				   const RiseFall *rf);
  TableModel *wireSlewDegradationTable(const RiseFall *rf) const;
  float degradeWireSlew(const RiseFall *rf,
			float in_slew,
			float wire_delay) const;
  // Check for supported axis variables.
  // Return true if axes are supported.
  static bool checkSlewDegradationAxes(const TablePtr &table);

  float defaultInputPinCap() const { return default_input_pin_cap_; }
  void setDefaultInputPinCap(float cap);

  float defaultOutputPinCap() const { return default_output_pin_cap_; }
  void setDefaultOutputPinCap(float cap);

  float defaultBidirectPinCap() const { return default_bidirect_pin_cap_; }
  void setDefaultBidirectPinCap(float cap);

  void defaultIntrinsic(const RiseFall *rf,
                        // Return values.
                        float &intrisic,
                        bool &exists) const;
  void setDefaultIntrinsic(const RiseFall *rf,
			   float value);
  // Uses defaultOutputPinRes or defaultBidirectPinRes based on dir.
  void defaultPinResistance(const RiseFall *rf,
			    const PortDirection *dir,
			    // Return values.
			    float &res,
			    bool &exists) const;
  void defaultBidirectPinRes(const RiseFall *rf,
			     // Return values.
			     float &res,
			     bool &exists) const;
  void setDefaultBidirectPinRes(const RiseFall *rf,
				float value);

  void defaultOutputPinRes(const RiseFall *rf,
			   // Return values.
			   float &res,
			   bool &exists) const;
  void setDefaultOutputPinRes(const RiseFall *rf,
			      float value);

  void defaultMaxSlew(float &slew,
		      bool &exists) const;
  void setDefaultMaxSlew(float slew);

  void defaultMaxCapacitance(float &cap,
			     bool &exists) const;
  void setDefaultMaxCapacitance(float cap);

  void defaultMaxFanout(float &fanout,
			bool &exists) const;
  void setDefaultMaxFanout(float fanout);

  void defaultFanoutLoad(// Return values.
			 float &fanout,
			 bool &exists) const;
  void setDefaultFanoutLoad(float load);

  // Logic thresholds.
  float inputThreshold(const RiseFall *rf) const;
  void setInputThreshold(const RiseFall *rf,
			 float th);
  float outputThreshold(const RiseFall *rf) const;
  void setOutputThreshold(const RiseFall *rf,
			  float th);
  // Slew thresholds (measured).
  float slewLowerThreshold(const RiseFall *rf) const;
  void setSlewLowerThreshold(const RiseFall *rf,
			     float th);
  float slewUpperThreshold(const RiseFall *rf) const;
  void setSlewUpperThreshold(const RiseFall *rf,
			     float th);
  // The library and delay calculator use the liberty slew upper/lower
  // (measured) thresholds for the table axes and value.  These slews
  // are scaled by slew_derate_from_library to get slews reported to
  // the user.
  // slew(measured) = slew_derate_from_library * slew(table)
  //   measured is from slew_lower_threshold to slew_upper_threshold
  float slewDerateFromLibrary() const;
  void setSlewDerateFromLibrary(float derate);

  Units *units() { return units_; }
  const Units *units() const { return units_; }

  Wireload *findWireload(const char *name) const;
  void setDefaultWireload(Wireload *wireload);
  Wireload *defaultWireload() const;
  WireloadSelection *findWireloadSelection(const char *name) const;
  WireloadSelection *defaultWireloadSelection() const;
  void addWireload(Wireload *wireload);
  WireloadMode defaultWireloadMode() const;
  void setDefaultWireloadMode(WireloadMode mode);
  void addWireloadSelection(WireloadSelection *selection);
  void setDefaultWireloadSelection(WireloadSelection *selection);

  OperatingConditions *findOperatingConditions(const char *name);
  OperatingConditions *defaultOperatingConditions() const;
  void addOperatingConditions(OperatingConditions *op_cond);
  void setDefaultOperatingConditions(OperatingConditions *op_cond);

  // AOCV
  // Zero means the ocv depth is not specified.
  float ocvArcDepth() const;
  void setOcvArcDepth(float depth);
  OcvDerate *defaultOcvDerate() const;
  void setDefaultOcvDerate(OcvDerate *derate);
  OcvDerate *findOcvDerate(const char *derate_name);
  void addOcvDerate(OcvDerate *derate);
  void addSupplyVoltage(const char *suppy_name,
			float voltage);
  bool supplyExists(const char *suppy_name) const;
  void supplyVoltage(const char *supply_name,
		     // Return value.
		     float &voltage,
		     bool &exists) const;

  // Make scaled cell.  Call LibertyCell::addScaledCell after it is complete.
  LibertyCell *makeScaledCell(const char *name,
			      const char *filename);

  static void
  makeCornerMap(LibertyLibrary *lib,
		int ap_index,
		Network *network,
		Report *report);
  static void
  makeCornerMap(LibertyCell *link_cell,
		LibertyCell *map_cell,
		int ap_index,
		Report *report);
  static void
  makeCornerMap(LibertyCell *cell1,
		LibertyCell *cell2,
		bool link,
		int ap_index,
		Report *report);
  static void
  checkCorners(LibertyCell *cell,
               Corners *corners,
               Report *report);

  DriverWaveform *findDriverWaveform(const char *name);
  DriverWaveform *driverWaveformDefault() { return driver_waveform_default_; }
  void addDriverWaveform(DriverWaveform *driver_waveform);

protected:
  float degradeWireSlew(const TableModel *model,
			float in_slew,
			float wire_delay) const;

  Units *units_;
  DelayModelType delay_model_type_;
  BusDclMap bus_dcls_;
  TableTemplateMap template_maps_[table_template_type_count];
  float nominal_process_;
  float nominal_voltage_;
  float nominal_temperature_;
  ScaleFactors *scale_factors_;
  ScaleFactorsMap scale_factors_map_;
  TableModel *wire_slew_degradation_tbls_[RiseFall::index_count];
  float default_input_pin_cap_;
  float default_output_pin_cap_;
  float default_bidirect_pin_cap_;
  RiseFallValues default_intrinsic_;
  RiseFallValues default_inout_pin_res_;
  RiseFallValues default_output_pin_res_;
  float default_fanout_load_;
  bool default_fanout_load_exists_;
  float default_max_cap_;
  bool default_max_cap_exists_;
  float default_max_fanout_;
  bool default_max_fanout_exists_;
  float default_max_slew_;
  bool default_max_slew_exists_;
  float input_threshold_[RiseFall::index_count];
  float output_threshold_[RiseFall::index_count];
  float slew_lower_threshold_[RiseFall::index_count];
  float slew_upper_threshold_[RiseFall::index_count];
  float slew_derate_from_library_;
  WireloadMap wireloads_;
  Wireload *default_wire_load_;
  WireloadMode default_wire_load_mode_;
  WireloadSelection *default_wire_load_selection_;
  WireloadSelectionMap wire_load_selections_;
  OperatingConditionsMap operating_conditions_;
  OperatingConditions *default_operating_conditions_;
  float ocv_arc_depth_;
  OcvDerate *default_ocv_derate_;
  OcvDerateMap ocv_derate_map_;
  SupplyVoltageMap supply_voltage_map_;
  LibertyCellSeq *buffers_;
  LibertyCellSeq *inverters_;
  DriverWaveformMap driver_waveform_map_;
  // Unnamed driver waveform.
  DriverWaveform *driver_waveform_default_;

  static constexpr float input_threshold_default_ = .5;
  static constexpr float output_threshold_default_ = .5;
  static constexpr float slew_lower_threshold_default_ = .2;
  static constexpr float slew_upper_threshold_default_ = .8;

private:
  friend class LibertyCell;
  friend class LibertyCellIterator;
};

class LibertyCellIterator : public Iterator<LibertyCell*>
{
public:
  explicit LibertyCellIterator(const LibertyLibrary *library);
  bool hasNext();
  LibertyCell *next();

private:
  ConcreteCellMap::ConstIterator iter_;
};

////////////////////////////////////////////////////////////////

class LibertyCell : public ConcreteCell
{
public:
  LibertyCell(LibertyLibrary *library,
	      const char *name,
	      const char *filename);
  virtual ~LibertyCell();
  LibertyLibrary *libertyLibrary() const { return liberty_library_; }
  LibertyLibrary *libertyLibrary() { return liberty_library_; }
  LibertyPort *findLibertyPort(const char *name) const;
  LibertyPortSeq findLibertyPortsMatching(PatternMatch *pattern) const;
  bool hasInternalPorts() const { return has_internal_ports_; }
  LibertyPgPort *findPgPort(const char *name) const;
  size_t pgPortCount() const { return pg_port_map_.size(); }
  ScaleFactors *scaleFactors() const { return scale_factors_; }
  void setScaleFactors(ScaleFactors *scale_factors);
  ModeDef *makeModeDef(const char *name);
  ModeDef *findModeDef(const char *name);

  float area() const { return area_; }
  void setArea(float area);
  bool dontUse() const { return dont_use_; }
  void setDontUse(bool dont_use);
  bool isMacro() const { return is_macro_; }
  void setIsMacro(bool is_macro);
  bool isMemory() const { return is_memory_; }
  void setIsMemory(bool is_memory);
  bool isPad() const { return is_pad_; }
  void setIsPad(bool is_pad);
  bool isClockCell() const { return is_clock_cell_; }
  void setIsClockCell(bool is_clock_cell);
  bool isLevelShifter() const { return is_level_shifter_; }
  void setIsLevelShifter(bool is_level_shifter);
  LevelShifterType levelShifterType() const { return level_shifter_type_; }
  void setLevelShifterType(LevelShifterType level_shifter_type);
  bool isIsolationCell() const { return is_isolation_cell_; }
  void setIsIsolationCell(bool is_isolation_cell);
  bool alwaysOn() const { return always_on_; }
  void setAlwaysOn(bool always_on);
  SwitchCellType switchCellType() const { return switch_cell_type_; }
  void setSwitchCellType(SwitchCellType switch_cell_type);
  bool interfaceTiming() const { return interface_timing_; }
  void setInterfaceTiming(bool value);
  bool isClockGateLatchPosedge() const;
  bool isClockGateLatchNegedge() const;
  bool isClockGateOther() const;
  bool isClockGate() const;
  void setClockGateType(ClockGateType type);
  const TimingArcSetSeq &timingArcSets() const { return timing_arc_sets_; }
  // from or to may be nullptr to wildcard.
  const TimingArcSetSeq &timingArcSets(const LibertyPort *from,
                                       const LibertyPort *to) const;
  size_t timingArcSetCount() const;
  // Find a timing arc set equivalent to key.
  TimingArcSet *findTimingArcSet(TimingArcSet *key) const;
  TimingArcSet *findTimingArcSet(unsigned arc_set_index) const;
  bool hasTimingArcs(LibertyPort *port) const;

  const InternalPowerSeq &internalPowers() const { return internal_powers_; }
  const InternalPowerSeq &internalPowers(const LibertyPort *port);
  LeakagePowerSeq *leakagePowers() { return &leakage_powers_; }
  void leakagePower(// Return values.
		    float &leakage,
		    bool &exists) const;
  bool leakagePowerExists() const { return leakage_power_exists_; }

  // Register, Latch or Statetable.
  bool hasSequentials() const;
  const SequentialSeq &sequentials() const { return sequentials_; }
  // Find the sequential with the output connected to an (internal) port.
  Sequential *outputPortSequential(LibertyPort *port);
  const Statetable *statetable() const { return statetable_; }

  // Find bus declaration local to this cell.
  BusDcl *findBusDcl(const char *name) const;
  // True when TimingArcSetBuilder::makeRegLatchArcs infers register
  // timing arcs.
  bool hasInferedRegTimingArcs() const { return has_infered_reg_timing_arcs_; }
  TestCell *testCell() const { return test_cell_; }
  bool isLatchData(LibertyPort *port);
  void latchEnable(const TimingArcSet *arc_set,
		   // Return values.
		   const LibertyPort *&enable_port,
		   const FuncExpr *&enable_func,
		   const RiseFall *&enable_rf) const;
  const RiseFall *latchCheckEnableEdge(TimingArcSet *check_set);
  bool isDisabledConstraint() const { return is_disabled_constraint_; }
  LibertyCell *cornerCell(const Corner *corner,
                          const MinMax *min_max);
  LibertyCell *cornerCell(const DcalcAnalysisPt *dcalc_ap);
  LibertyCell *cornerCell(int ap_index);

  // AOCV
  float ocvArcDepth() const;
  OcvDerate *ocvDerate() const;
  OcvDerate *findOcvDerate(const char *derate_name);

  // Build helpers.
  void makeSequential(int size,
		      bool is_register,
		      FuncExpr *clk,
		      FuncExpr *data,
		      FuncExpr *clear,
		      FuncExpr *preset,
		      LogicValue clr_preset_out,
		      LogicValue clr_preset_out_inv,
		      LibertyPort *output,
		      LibertyPort *output_inv);
  void makeStatetable(LibertyPortSeq &input_ports,
                      LibertyPortSeq &internal_ports,
                      StatetableRows &table);
  void addBusDcl(BusDcl *bus_dcl);
  // Add scaled cell after it is complete.
  void addScaledCell(OperatingConditions *op_cond,
		     LibertyCell *scaled_cell);
  unsigned addTimingArcSet(TimingArcSet *set);
  void addInternalPower(InternalPower *power);
  void addInternalPowerAttrs(InternalPowerAttrs *attrs);
  void addLeakagePower(LeakagePower *power);
  void setLeakagePower(float leakage);
  void setOcvArcDepth(float depth);
  void setOcvDerate(OcvDerate *derate);
  void addOcvDerate(OcvDerate *derate);
  void addPgPort(LibertyPgPort *pg_port);
  void setTestCell(TestCell *test);
  void setHasInferedRegTimingArcs(bool infered);
  void setIsDisabledConstraint(bool is_disabled);
  void setCornerCell(LibertyCell *corner_cell,
		     int ap_index);
  // Call after cell is finished being constructed.
  void finish(bool infer_latches,
	      Report *report,
	      Debug *debug);
  bool isBuffer() const;
  bool isInverter() const;
  // Only valid when isBuffer() returns true.
  void bufferPorts(// Return values.
		   LibertyPort *&input,
		   LibertyPort *&output) const;
  // Check all liberty cells to make sure they exist
  // for all the defined corners.
  static void checkLibertyCorners();
  void ensureVoltageWaveforms(const DcalcAnalysisPtSeq &dcalc_aps);
  const char *footprint() const;
  void setFootprint(const char *footprint);
  const char *userFunctionClass() const;
  void setUserFunctionClass(const char *user_function_class);

protected:
  void addPort(ConcretePort *port);
  void setHasInternalPorts(bool has_internal);
  void setLibertyLibrary(LibertyLibrary *library);
  void makeLatchEnables(Report *report,
			Debug *debug);
  FuncExpr *findLatchEnableFunc(const LibertyPort *d,
                                const LibertyPort *en,
                                const RiseFall *en_rf) const;
  LatchEnable *makeLatchEnable(LibertyPort *d,
			       LibertyPort *en,
                               const RiseFall *en_rf,
			       LibertyPort *q,
			       TimingArcSet *d_to_q,
			       TimingArcSet *en_to_q,
			       TimingArcSet *setup_check,
			       Debug *debug);
  TimingArcSet *findLatchSetup(const LibertyPort *d,
                               const LibertyPort *en,
                               const RiseFall *en_rf,
                               const LibertyPort *q,
                               const TimingArcSet *en_to_q,
                               Report *report);
  bool condMatch(const TimingArcSet *arc_set1,
                 const TimingArcSet *arc_set2);
  void findDefaultCondArcs();
  void translatePresetClrCheckRoles();
  void inferLatchRoles(Report *report,
                       Debug *debug);
  void deleteInternalPowerAttrs();
  void makeTimingArcMap(Report *report);
  void makeTimingArcPortMaps();
  bool hasBufferFunc(const LibertyPort *input,
		     const LibertyPort *output) const;
  bool hasInverterFunc(const LibertyPort *input,
		       const LibertyPort *output) const;
  bool checkCornerCell(const Corner *corner,
                       const MinMax *min_max) const;

  LibertyLibrary *liberty_library_;
  float area_;
  bool dont_use_;
  bool is_macro_;
  bool is_memory_;
  bool is_pad_;
  bool is_clock_cell_;
  bool is_level_shifter_;
  LevelShifterType level_shifter_type_;
  bool is_isolation_cell_;
  bool always_on_;
  SwitchCellType switch_cell_type_;
  bool interface_timing_;
  ClockGateType clock_gate_type_;
  TimingArcSetSeq timing_arc_sets_;
  TimingArcSetMap timing_arc_set_map_;
  LibertyPortPairTimingArcMap port_timing_arc_set_map_;
  LibertyPortTimingArcMap timing_arc_set_from_map_;
  LibertyPortTimingArcMap timing_arc_set_to_map_;
  bool has_infered_reg_timing_arcs_;
  InternalPowerSeq internal_powers_;
  PortInternalPowerSeq port_internal_powers_;
  InternalPowerAttrsSeq internal_power_attrs_;
  LeakagePowerSeq leakage_powers_;
  SequentialSeq sequentials_;
  PortToSequentialMap port_to_seq_map_;
  Statetable *statetable_;
  BusDclMap bus_dcls_;
  ModeDefMap mode_defs_;
  ScaleFactors *scale_factors_;
  ScaledCellMap scaled_cells_;
  TestCell *test_cell_;
  // Latch D->Q to LatchEnable.
  LatchEnableMap latch_d_to_q_map_;
  // Latch EN->D setup to LatchEnable.
  LatchEnableMap latch_check_map_;
  LatchEnableSeq latch_enables_;
  // Ports that have latch D->Q timing arc sets from them.
  LibertyPortSet latch_data_ports_;
  float ocv_arc_depth_;
  OcvDerate *ocv_derate_;
  OcvDerateMap ocv_derate_map_;
  bool is_disabled_constraint_;
  Vector<LibertyCell*> corner_cells_;
  float leakage_power_;
  bool leakage_power_exists_;
  LibertyPgPortMap pg_port_map_;
  bool has_internal_ports_;
  std::atomic<bool> have_voltage_waveforms_;
  std::mutex waveform_lock_;
  std::string footprint_;
  std::string user_function_class_;

private:
  friend class LibertyLibrary;
  friend class LibertyCellPortIterator;
  friend class LibertyCellPgPortIterator;
  friend class LibertyPort;
  friend class LibertyBuilder;
};

class LibertyCellPortIterator : public Iterator<LibertyPort*>
{
public:
  explicit LibertyCellPortIterator(const LibertyCell *cell);
  bool hasNext();
  LibertyPort *next();

private:
  ConcretePortSeq::ConstIterator iter_;
};

class LibertyCellPortBitIterator : public Iterator<LibertyPort*>
{
public:
  explicit LibertyCellPortBitIterator(const LibertyCell *cell);
  virtual ~LibertyCellPortBitIterator();
  bool hasNext();
  LibertyPort *next();

private:
  ConcreteCellPortBitIterator *iter_;
};

class LibertyCellPgPortIterator : public Iterator<LibertyPgPort*>
{
public:
  LibertyCellPgPortIterator(const LibertyCell *cell);
  bool hasNext();
  LibertyPgPort *next();

private:
  LibertyPgPortMap::Iterator iter_;
};

////////////////////////////////////////////////////////////////

class LibertyPort : public ConcretePort
{
public:
  LibertyCell *libertyCell() const { return liberty_cell_; }
  LibertyLibrary *libertyLibrary() const { return liberty_cell_->libertyLibrary(); }
  LibertyPort *findLibertyMember(int index) const;
  LibertyPort *findLibertyBusBit(int index) const;
  LibertyPort *bundlePort() const;
  BusDcl *busDcl() const { return bus_dcl_; }
  void setDirection(PortDirection *dir);
  ScanSignalType scanSignalType() const { return scan_signal_type_; }
  void setScanSignalType(ScanSignalType type);
  void fanoutLoad(// Return values.
		  float &fanout_load,
		  bool &exists) const;
  void setFanoutLoad(float fanout_load);
  float capacitance() const;
  float capacitance(const MinMax *min_max) const;
  float capacitance(const RiseFall *rf,
		    const MinMax *min_max) const;
  void capacitance(const RiseFall *rf,
		   const MinMax *min_max,
		   // Return values.
		   float &cap,
		   bool &exists) const;
  // Capacitance at op_cond derated by library/cell scale factors
  // using pvt.
  float capacitance(const RiseFall *rf,
		    const MinMax *min_max,
		    const OperatingConditions *op_cond,
		    const Pvt *pvt) const;
  bool capacitanceIsOneValue() const;
  void setCapacitance(float cap);
  void setCapacitance(const RiseFall *rf,
		      const MinMax *min_max,
		      float cap);
  // Max of rise/fall.
  float driveResistance() const;
  float driveResistance(const RiseFall *rf,
			const MinMax *min_max) const;
  // Zero load delay.
  ArcDelay intrinsicDelay(const StaState *sta) const;
  ArcDelay intrinsicDelay(const RiseFall *rf,
                          const MinMax *min_max,
                          const StaState *sta) const;
  FuncExpr *function() const { return function_; }
  void setFunction(FuncExpr *func);
  // Tristate enable function.
  FuncExpr *tristateEnable() const { return tristate_enable_; }
  void setTristateEnable(FuncExpr *enable);
  void slewLimit(const MinMax *min_max,
		 // Return values.
		 float &limit,
		 bool &exists) const;
  void setSlewLimit(float slew,
		    const MinMax *min_max);
  void capacitanceLimit(const MinMax *min_max,
			// Return values.
			float &limit,
			bool &exists) const;
  void setCapacitanceLimit(float cap,
			   const MinMax *min_max);
  void fanoutLimit(const MinMax *min_max,
		   // Return values.
		   float &limit,
		   bool &exists) const;
  void setFanoutLimit(float fanout,
		      const MinMax *min_max);
  void minPeriod(const OperatingConditions *op_cond,
		 const Pvt *pvt,
		 float &min_period,
		 bool &exists) const;
  // Unscaled value.
  void minPeriod(float &min_period,
		 bool &exists) const;
  void setMinPeriod(float min_period);
  // This corresponds to the min_pulse_width_high/low port attribute.
  // high = rise, low = fall
  void minPulseWidth(const RiseFall *hi_low,
		     float &min_width,
		     bool &exists) const;
  void setMinPulseWidth(const RiseFall *hi_low,
			float min_width);
  bool isClock() const;
  void setIsClock(bool is_clk);
  bool isClockGateClock() const { return is_clk_gate_clk_; }
  void setIsClockGateClock(bool is_clk_gate_clk);
  bool isClockGateEnable() const { return is_clk_gate_enable_; }
  void setIsClockGateEnable(bool is_clk_gate_enable);
  bool isClockGateOut() const { return is_clk_gate_out_; }
  void setIsClockGateOut(bool is_clk_gate_out);
  bool isPllFeedback() const { return is_pll_feedback_; }
  void setIsPllFeedback(bool is_pll_feedback);

  bool isolationCellData() const { return isolation_cell_data_; }
  void setIsolationCellData(bool isolation_cell_data);

  bool isolationCellEnable() const { return isolation_cell_enable_; }
  void setIsolationCellEnable(bool isolation_cell_enable);

  bool levelShifterData() const { return level_shifter_data_; }
  void setLevelShifterData(bool level_shifter_data);

  bool isSwitch() const { return is_switch_; }
  void setIsSwitch(bool is_switch);

  // Has register/latch rise/fall edges from pin.
  bool isRegClk() const { return is_reg_clk_; }
  void setIsRegClk(bool is_clk);
  // Is the clock for timing checks.
  bool isCheckClk() const { return is_check_clk_; }
  void setIsCheckClk(bool is_clk);
  bool isPad() const { return is_pad_; }
  void setIsPad(bool is_pad);
  const RiseFall *pulseClkTrigger() const { return pulse_clk_trigger_; }
  // Rise for high, fall for low.
  const RiseFall *pulseClkSense() const { return pulse_clk_sense_; }
  void setPulseClk(const RiseFall *rfigger,
		   const RiseFall *sense);
  bool isDisabledConstraint() const { return is_disabled_constraint_; }
  void setIsDisabledConstraint(bool is_disabled);
  LibertyPort *cornerPort(const Corner *corner,
                          const MinMax *min_max);
  const LibertyPort *cornerPort(const Corner *corner,
                                const MinMax *min_max) const;
  LibertyPort *cornerPort(const DcalcAnalysisPt *dcalc_ap);
  const LibertyPort *cornerPort(const DcalcAnalysisPt *dcalc_ap) const;
  LibertyPort *cornerPort(int ap_index);
  const LibertyPort *cornerPort(int ap_index) const;
  void setCornerPort(LibertyPort *corner_port,
		     int ap_index);
  const char *relatedGroundPin() const;
  void setRelatedGroundPin(const char *related_ground_pin);
  const char *relatedPowerPin() const;
  void setRelatedPowerPin(const char *related_power_pin);
  const ReceiverModel *receiverModel() const { return receiver_model_.get(); }
  void setReceiverModel(ReceiverModelPtr receiver_model);
  DriverWaveform *driverWaveform(const RiseFall *rf) const;
  void setDriverWaveform(DriverWaveform *driver_waveform,
                         const RiseFall *rf);
  float clkTreeDelay(float in_slew,
                     const RiseFall *from_rf,
                     const RiseFall *to_rf,
                     const MinMax *min_max) const;
  float clkTreeDelay(float in_slew,
                     const RiseFall *from_rf,
                     const MinMax *min_max) const;
  void setClkTreeDelay(const TableModel *model,
                       const RiseFall *from_rf,
                       const RiseFall *to_rf,
                       const MinMax *min_max);
  // deprecated 2024-06-22
  RiseFallMinMax clkTreeDelays() const __attribute__ ((deprecated));
  // deprecated 2024-02-27
  RiseFallMinMax clockTreePathDelays() const __attribute__ ((deprecated));

  static bool equiv(const LibertyPort *port1,
		    const LibertyPort *port2);
  static bool less(const LibertyPort *port1,
		   const LibertyPort *port2);

protected:
  // Constructor is internal to LibertyBuilder.
  LibertyPort(LibertyCell *cell,
	      const char *name,
	      bool is_bus,
	      BusDcl *bus_dcl,
              int from_index,
	      int to_index,
	      bool is_bundle,
	      ConcretePortSeq *members);
  virtual ~LibertyPort();
  void setMinPort(LibertyPort *min);
  void addScaledPort(OperatingConditions *op_cond,
		     LibertyPort *scaled_port);
  RiseFallMinMax clkTreeDelays1() const;

  LibertyCell *liberty_cell_;
  BusDcl *bus_dcl_;
  FuncExpr *function_;
  ScanSignalType scan_signal_type_;
  FuncExpr *tristate_enable_;
  ScaledPortMap *scaled_ports_;
  RiseFallMinMax capacitance_;
  MinMaxFloatValues slew_limit_;   // inputs and outputs
  MinMaxFloatValues cap_limit_;    // outputs
  float fanout_load_;              // inputs
  bool fanout_load_exists_;
  MinMaxFloatValues fanout_limit_; // outputs
  float min_period_;
  float min_pulse_width_[RiseFall::index_count];
  const RiseFall *pulse_clk_trigger_;
  const RiseFall *pulse_clk_sense_;
  std::string related_ground_pin_;
  std::string related_power_pin_;
  Vector<LibertyPort*> corner_ports_;
  ReceiverModelPtr receiver_model_;
  DriverWaveform *driver_waveform_[RiseFall::index_count];
  // Redundant with clock_tree_path_delay timing arcs but faster to access.
  const TableModel *clk_tree_delay_[RiseFall::index_count][RiseFall::index_count][MinMax::index_count];

  unsigned int min_pulse_width_exists_:RiseFall::index_count;
  bool min_period_exists_:1;
  bool is_clk_:1;
  bool is_reg_clk_:1;
  bool is_check_clk_:1;
  bool is_clk_gate_clk_:1;
  bool is_clk_gate_enable_:1;
  bool is_clk_gate_out_:1;
  bool is_pll_feedback_:1;
  bool isolation_cell_data_:1;
  bool isolation_cell_enable_:1;
  bool level_shifter_data_:1;
  bool is_switch_:1;
  bool is_disabled_constraint_:1;
  bool is_pad_:1;

private:
  friend class LibertyLibrary;
  friend class LibertyCell;
  friend class LibertyBuilder;
  friend class LibertyReader;
};

LibertyPortSeq
sortByName(const LibertyPortSet *set);

class LibertyPortMemberIterator : public Iterator<LibertyPort*>
{
public:
  explicit LibertyPortMemberIterator(const LibertyPort *port);
  virtual ~LibertyPortMemberIterator();
  virtual bool hasNext();
  virtual LibertyPort *next();

private:
  ConcretePortMemberIterator *iter_;
};

// Process, voltage temperature.
class Pvt
{
public:
  Pvt(float process,
      float voltage,
      float temperature);
  virtual ~Pvt() {}
  float process() const { return process_; }
  void setProcess(float process);
  float voltage() const { return voltage_; }
  void setVoltage(float voltage);
  float temperature() const { return temperature_; }
  void setTemperature(float temp);

protected:
  float process_;
  float voltage_;
  float temperature_;
};

class OperatingConditions : public Pvt
{
public:
  OperatingConditions(const char *name);
  OperatingConditions(const char *name,
		      float process,
		      float voltage,
		      float temperature,
		      WireloadTree wire_load_tree);
  const char *name() const { return name_.c_str(); }
  WireloadTree wireloadTree() const { return wire_load_tree_; }
  void setWireloadTree(WireloadTree tree);

protected:
  std::string name_;
  WireloadTree wire_load_tree_;
};

class ScaleFactors
{
public:
  explicit ScaleFactors(const char *name);
  const char *name() const { return name_.c_str(); }
  float scale(ScaleFactorType type,
	      ScaleFactorPvt pvt,
	      const RiseFall *rf);
  float scale(ScaleFactorType type,
	      ScaleFactorPvt pvt,
	      int rf_index);
  float scale(ScaleFactorType type,
	      ScaleFactorPvt pvt);
  void setScale(ScaleFactorType type,
		ScaleFactorPvt pvt,
		const RiseFall *rf,
		float scale);
  void setScale(ScaleFactorType type,
		ScaleFactorPvt pvt,
		float scale);
  void print();

protected:
  std::string name_;
  float scales_[scale_factor_type_count][scale_factor_pvt_count][RiseFall::index_count];
};

class BusDcl
{
public:
  BusDcl(const char *name,
	 int from,
	 int to);
  const char *name() const { return name_.c_str(); }
  int from() const { return from_; }
  int to() const { return to_; }

protected:
  std::string name_;
  int from_;
  int to_;
};

// Cell mode_definition group.
class ModeDef
{
public:
  ~ModeDef();
  const char *name() const { return name_.c_str(); }
  ModeValueDef *defineValue(const char *value,
			    FuncExpr *cond,
			    const char *sdf_cond);
  ModeValueDef *findValueDef(const char *value);
  ModeValueMap *values() { return &values_; }

protected:
  // Private to LibertyCell::makeModeDef.
  ModeDef(const char *name);

  std::string name_;
  ModeValueMap values_;

private:
  friend class LibertyCell;
};

// Mode definition mode_value group.
class ModeValueDef
{
public:
  ~ModeValueDef();
  const char *value() const { return value_.c_str(); }
  FuncExpr *cond() const { return cond_; }
  void setCond(FuncExpr *cond);
  const char *sdfCond() const { return sdf_cond_.c_str(); }
  void setSdfCond(const char *sdf_cond);

protected:
  // Private to ModeDef::defineValue.
  ModeValueDef(const char *value,
	       FuncExpr *cond,
	       const char *sdf_cond);

  std::string value_;
  FuncExpr *cond_;
  std::string sdf_cond_;

private:
  friend class ModeDef;
};

class TableTemplate
{
public:
  TableTemplate(const char *name);
  TableTemplate(const char *name,
		TableAxisPtr axis1,
		TableAxisPtr axis2,
		TableAxisPtr axis3);
  const char *name() const { return name_.c_str(); }
  void setName(const char *name);
  const TableAxis *axis1() const { return axis1_.get(); }
  TableAxisPtr axis1ptr() const { return axis1_; }
  void setAxis1(TableAxisPtr axis);
  const TableAxis *axis2() const { return axis2_.get(); }
  TableAxisPtr axis2ptr() const { return axis2_; }
  void setAxis2(TableAxisPtr axis);
  const TableAxis *axis3() const { return axis3_.get(); }
  TableAxisPtr axis3ptr() const { return axis3_; }
  void setAxis3(TableAxisPtr axis);

protected:
  std::string name_;
  TableAxisPtr axis1_;
  TableAxisPtr axis2_;
  TableAxisPtr axis3_;
};

class TestCell : public LibertyCell
{
public:
  TestCell(LibertyLibrary *library,
           const char *name,
           const char *filename);

protected:
};

class OcvDerate
{
public:
  OcvDerate(const char *name);
  ~OcvDerate();
  const char *name() const { return name_; }
  const Table *derateTable(const RiseFall *rf,
                           const EarlyLate *early_late,
                           PathType path_type);
  void setDerateTable(const RiseFall *rf,
		      const EarlyLate *early_late,
		      PathType path_type,
		      TablePtr derate);

private:
  const char *name_;
  // [rf_type][derate_type][path_type]
  TablePtr derate_[RiseFall::index_count][EarlyLate::index_count][path_type_count];
};

// Power/ground port.
class LibertyPgPort
{
public:
  enum PgType { unknown,
		primary_power, primary_ground,
		backup_power, backup_ground,
		internal_power, internal_ground,
		nwell, pwell,
		deepnwell, deeppwell};
  LibertyPgPort(const char *name,
		LibertyCell *cell);
  const char *name() const { return name_.c_str(); }
  LibertyCell *cell() const { return cell_; }
  PgType pgType() const { return pg_type_; }
  void setPgType(PgType type);
  const char *voltageName() const { return voltage_name_.c_str(); }
  void setVoltageName(const char *voltage_name);
  static bool equiv(const LibertyPgPort *port1,
		    const LibertyPgPort *port2);

private:
  std::string name_;
  PgType pg_type_;
  std::string voltage_name_;
  LibertyCell *cell_;
};

std::string
portLibertyToSta(const char *port_name);
const char *
scanSignalTypeName(ScanSignalType scan_type);

} // namespace
