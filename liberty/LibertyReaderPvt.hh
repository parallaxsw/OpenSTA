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

#include <functional>
#include <vector>

#include "Vector.hh"
#include "Map.hh"
#include "StringSeq.hh"
#include "MinMax.hh"
#include "NetworkClass.hh"
#include "Transition.hh"
#include "TimingArc.hh"
#include "InternalPower.hh"
#include "LeakagePower.hh"
#include "Liberty.hh"
#include "Sequential.hh"
#include "TableModel.hh"
#include "LibertyParser.hh"
#include "LibertyReader.hh"
#include "LibertyBuilder.hh"

namespace sta {

class LibertyBuilder;
class LibertyReader;
class LibertyFunc;
class PortGroup;
class SequentialGroup;
class StatetableGroup;
class RelatedPortGroup;
class TimingGroup;
class InternalPowerGroup;
class LeakagePowerGroup;
class PortNameBitIterator;
class TimingArcBuilder;
class LibertyAttr;
class OutputWaveform;

typedef void (LibertyReader::*LibraryAttrVisitor)(LibertyAttr *attr);
typedef void (LibertyReader::*LibraryGroupVisitor)(LibertyGroup *group);
typedef Map<std::string, LibraryAttrVisitor> LibraryAttrMap;
typedef Map<std::string ,LibraryGroupVisitor> LibraryGroupMap;
typedef Vector<PortGroup*> PortGroupSeq;
typedef Vector<SequentialGroup*> SequentialGroupSeq;
typedef Vector<LibertyFunc*> LibertyFuncSeq;
typedef Vector<TimingGroup*> TimingGroupSeq;
typedef Vector<InternalPowerGroup*> InternalPowerGroupSeq;
typedef Vector<LeakagePowerGroup*> LeakagePowerGroupSeq;
typedef void (LibertyPort::*LibertyPortBoolSetter)(bool value);
typedef Vector<OutputWaveform*> OutputWaveformSeq;
typedef std::vector<std::string> StdStringSeq;
typedef std::function<void (FuncExpr *expr)> LibertySetFunc;

class LibertyReader : public LibertyGroupVisitor
{
public:
  LibertyReader(const char *filename,
                bool infer_latches,
                Network *network);
  virtual ~LibertyReader();
  virtual LibertyLibrary *readLibertyFile(const char *filename);
  virtual void init(const char *filename,
                    bool infer_latches,
                    Network *network);
  LibertyLibrary *library() const { return library_; }
  virtual bool save(LibertyGroup *) { return false; }
  virtual bool save(LibertyAttr *) { return false; }
  virtual bool save(LibertyVariable *) { return false; }

  virtual void beginLibrary(LibertyGroup *group);
  virtual void endLibrary(LibertyGroup *group);
  virtual void endLibraryAttrs(LibertyGroup *group);
  virtual void visitAttr(LibertyAttr *attr);
  virtual void visitTimeUnit(LibertyAttr *attr);
  virtual void visitCapacitiveLoadUnit(LibertyAttr *attr);
  virtual void visitResistanceUnit(LibertyAttr *attr);
  virtual void visitPullingResistanceUnit(LibertyAttr *attr);
  virtual void visitVoltageUnit(LibertyAttr *attr);
  virtual void visitCurrentUnit(LibertyAttr *attr);
  virtual void visitPowerUnit(LibertyAttr *attr);
  virtual void visitDistanceUnit(LibertyAttr *attr);
  virtual void parseUnits(LibertyAttr *attr,
			  const char *suffix,
			  float &scale_var,
			  Unit *unit_suffix);
  virtual void visitDelayModel(LibertyAttr *attr);
  virtual void visitVoltageMap(LibertyAttr *attr);
  virtual void visitBusStyle(LibertyAttr *attr);
  virtual void visitNomTemp(LibertyAttr *attr);
  virtual void visitNomVolt(LibertyAttr *attr);
  virtual void visitNomProc(LibertyAttr *attr);
  virtual void visitDefaultInoutPinCap(LibertyAttr *attr);
  virtual void visitDefaultInputPinCap(LibertyAttr *attr);
  virtual void visitDefaultOutputPinCap(LibertyAttr *attr);
  virtual void visitDefaultMaxTransition(LibertyAttr *attr);
  virtual void visitDefaultMaxFanout(LibertyAttr *attr);
  virtual void visitDefaultIntrinsicRise(LibertyAttr *attr);
  virtual void visitDefaultIntrinsicFall(LibertyAttr *attr);
  virtual void visitDefaultIntrinsic(LibertyAttr *attr,
				     const RiseFall *rf);
  virtual void visitDefaultInoutPinRiseRes(LibertyAttr *attr);
  virtual void visitDefaultInoutPinFallRes(LibertyAttr *attr);
  virtual void visitDefaultInoutPinRes(LibertyAttr *attr,
				       const RiseFall *rf);
  virtual void visitDefaultOutputPinRiseRes(LibertyAttr *attr);
  virtual void visitDefaultOutputPinFallRes(LibertyAttr *attr);
  virtual void visitDefaultOutputPinRes(LibertyAttr *attr,
					const RiseFall *rf);
  virtual void visitDefaultFanoutLoad(LibertyAttr *attr);
  virtual void visitDefaultWireLoad(LibertyAttr *attr);
  virtual void visitDefaultWireLoadMode(LibertyAttr *attr);
  virtual void visitDefaultWireLoadSelection(LibertyAttr *attr);
  virtual void visitDefaultOperatingConditions(LibertyAttr *attr);
  virtual void visitInputThresholdPctFall(LibertyAttr *attr);
  virtual void visitInputThresholdPctRise(LibertyAttr *attr);
  virtual void visitInputThresholdPct(LibertyAttr *attr,
				      const RiseFall *rf);
  virtual void visitOutputThresholdPctFall(LibertyAttr *attr);
  virtual void visitOutputThresholdPctRise(LibertyAttr *attr);
  virtual void visitOutputThresholdPct(LibertyAttr *attr,
				       const RiseFall *rf);
  virtual void visitSlewLowerThresholdPctFall(LibertyAttr *attr);
  virtual void visitSlewLowerThresholdPctRise(LibertyAttr *attr);
  virtual void visitSlewLowerThresholdPct(LibertyAttr *attr,
					  const RiseFall *rf);
  virtual void visitSlewUpperThresholdPctFall(LibertyAttr *attr);
  virtual void visitSlewUpperThresholdPctRise(LibertyAttr *attr);
  virtual void visitSlewUpperThresholdPct(LibertyAttr *attr,
					  const RiseFall *rf);
  virtual void visitSlewDerateFromLibrary(LibertyAttr *attr);

  virtual void beginTechnology(LibertyGroup *group);
  virtual void endTechnology(LibertyGroup *group);
  virtual void beginTableTemplateDelay(LibertyGroup *group);
  virtual void beginTableTemplateOutputCurrent(LibertyGroup *group);
  virtual void beginTableTemplate(LibertyGroup *group,
				  TableTemplateType type);
  virtual void endTableTemplate(LibertyGroup *group);
  virtual void visitVariable1(LibertyAttr *attr);
  virtual void visitVariable2(LibertyAttr *attr);
  virtual void visitVariable3(LibertyAttr *attr);
  virtual void visitIndex1(LibertyAttr *attr);
  virtual void visitIndex2(LibertyAttr *attr);
  virtual void visitIndex3(LibertyAttr *attr);

  virtual void beginType(LibertyGroup *group);
  virtual void endType(LibertyGroup *group);
  virtual void visitBitFrom(LibertyAttr *attr);
  virtual void visitBitTo(LibertyAttr *attr);

  virtual void beginCell(LibertyGroup *group);
  virtual void endCell(LibertyGroup *group);
  virtual void beginScaledCell(LibertyGroup *group);
  virtual void endScaledCell(LibertyGroup *group);
  virtual void checkScaledCell(LibertyGroup *group);
  virtual void finishPortGroups();
  virtual void checkPort(LibertyPort *port,
			 int line);
  virtual void makeTimingArcs(PortGroup *port_group);
  virtual void makeInternalPowers(PortGroup *port_group);
  virtual void makeCellSequentials();
  virtual void makeCellSequential(SequentialGroup *seq);
  virtual void makeStatetable();
  virtual void makeLeakagePowers();
  virtual void parseCellFuncs();
  virtual void makeLibertyFunc(const char *expr,
                               LibertySetFunc set_func,
			       bool invert,
			       const char *attr_name,
			       LibertyStmt *stmt);
  virtual void makeTimingArcs(LibertyPort *to_port,
			      TimingGroup *timing);
  virtual void makeTimingArcs(const char *from_port_name,
			      PortNameBitIterator &from_port_iter,
			      LibertyPort *to_port,
			      LibertyPort *related_out_port,
			      TimingGroup *timing);
  virtual void makeTimingArcs(LibertyPort *to_port,
                              LibertyPort *related_out_port,
			      TimingGroup *timing);

  virtual void visitClockGatingIntegratedCell(LibertyAttr *attr);
  virtual void visitArea(LibertyAttr *attr);
  virtual void visitDontUse(LibertyAttr *attr);
  virtual void visitIsMacro(LibertyAttr *attr);
  virtual void visitIsMemory(LibertyAttr *attr);
  virtual void visitIsPadCell(LibertyAttr *attr);
  virtual void visitIsPad(LibertyAttr *attr);
  virtual void visitIsClockCell(LibertyAttr *attr);
  virtual void visitIsLevelShifter(LibertyAttr *attr);
  virtual void visitLevelShifterType(LibertyAttr *attr);
  virtual void visitIsIsolationCell(LibertyAttr *attr);
  virtual void visitAlwaysOn(LibertyAttr *attr);
  virtual void visitSwitchCellType(LibertyAttr *attr);
  virtual void visitInterfaceTiming(LibertyAttr *attr);
  virtual void visitScalingFactors(LibertyAttr *attr);
  virtual void visitCellLeakagePower(LibertyAttr *attr);
  virtual void visitCellFootprint(LibertyAttr *attr);
  virtual void visitCellUserFunctionClass(LibertyAttr *attr);

  virtual void beginPin(LibertyGroup *group);
  virtual void endPin(LibertyGroup *group);
  virtual void beginBus(LibertyGroup *group);
  virtual void endBus(LibertyGroup *group);
  virtual void beginBundle(LibertyGroup *group);
  virtual void endBundle(LibertyGroup *group);
  virtual void beginBusOrBundle(LibertyGroup *group);
  virtual void endBusOrBundle();
  virtual void endPorts();
  virtual void setPortCapDefault(LibertyPort *port);
  virtual void visitMembers(LibertyAttr *attr);
  virtual void visitDirection(LibertyAttr *attr);
  virtual void visitFunction(LibertyAttr *attr);
  virtual void visitThreeState(LibertyAttr *attr);
  virtual void visitBusType(LibertyAttr *attr);
  virtual void visitCapacitance(LibertyAttr *attr);
  virtual void visitRiseCap(LibertyAttr *attr);
  virtual void visitFallCap(LibertyAttr *attr);
  virtual void visitRiseCapRange(LibertyAttr *attr);
  virtual void visitFallCapRange(LibertyAttr *attr);
  virtual void visitFanoutLoad(LibertyAttr *attr);
  virtual void visitMaxFanout(LibertyAttr *attr);
  virtual void visitMinFanout(LibertyAttr *attr);
  virtual void visitFanout(LibertyAttr *attr,
			   const MinMax *min_max);
  virtual void visitMaxTransition(LibertyAttr *attr);
  virtual void visitMinTransition(LibertyAttr *attr);
  virtual void visitMinMaxTransition(LibertyAttr *attr,
				     const MinMax *min_max);
  virtual void visitMaxCapacitance(LibertyAttr *attr);
  virtual void visitMinCapacitance(LibertyAttr *attr);
  virtual void visitMinMaxCapacitance(LibertyAttr *attr,
				      const MinMax *min_max);
  virtual void visitMinPeriod(LibertyAttr *attr);
  virtual void visitMinPulseWidthLow(LibertyAttr *attr);
  virtual void visitMinPulseWidthHigh(LibertyAttr *attr);
  virtual void visitMinPulseWidth(LibertyAttr *attr,
				  const RiseFall *rf);
  virtual void visitPulseClock(LibertyAttr *attr);
  virtual void visitClockGateClockPin(LibertyAttr *attr);
  virtual void visitClockGateEnablePin(LibertyAttr *attr);
  virtual void visitClockGateOutPin(LibertyAttr *attr);
  void visitIsPllFeedbackPin(LibertyAttr *attr);
  virtual void visitSignalType(LibertyAttr *attr);
  const EarlyLateAll *getAttrEarlyLate(LibertyAttr *attr);
  virtual void visitClock(LibertyAttr *attr);
  virtual void visitIsolationCellDataPin(LibertyAttr *attr);
  virtual void visitIsolationCellEnablePin(LibertyAttr *attr);
  virtual void visitLevelShifterDataPin(LibertyAttr *attr);
  virtual void visitSwitchPin(LibertyAttr *attr);
  void visitPortBoolAttr(LibertyAttr *attr,
                         LibertyPortBoolSetter setter);

  virtual void beginScalingFactors(LibertyGroup *group);
  virtual void endScalingFactors(LibertyGroup *group);
  virtual void defineScalingFactorVisitors();
  virtual void visitScaleFactorSuffix(LibertyAttr *attr);
  virtual void visitScaleFactorPrefix(LibertyAttr *attr);
  virtual void visitScaleFactorHiLow(LibertyAttr *attr);
  virtual void visitScaleFactor(LibertyAttr *attr);

  virtual void beginOpCond(LibertyGroup *group);
  virtual void endOpCond(LibertyGroup *group);
  virtual void visitProc(LibertyAttr *attr);
  virtual void visitVolt(LibertyAttr *attr);
  virtual void visitTemp(LibertyAttr *attr);
  virtual void visitTreeType(LibertyAttr *attr);

  virtual void beginWireload(LibertyGroup *group);
  virtual void endWireload(LibertyGroup *group);
  virtual void visitResistance(LibertyAttr *attr);
  virtual void visitSlope(LibertyAttr *attr);
  virtual void visitFanoutLength(LibertyAttr *attr);

  virtual void beginWireloadSelection(LibertyGroup *group);
  virtual void endWireloadSelection(LibertyGroup *group);
  virtual void visitWireloadFromArea(LibertyAttr *attr);

  virtual void beginMemory(LibertyGroup *group);
  virtual void endMemory(LibertyGroup *group);

  virtual void beginFF(LibertyGroup *group);
  virtual void endFF(LibertyGroup *group);
  virtual void beginFFBank(LibertyGroup *group);
  virtual void endFFBank(LibertyGroup *group);
  virtual void beginLatch(LibertyGroup *group);
  virtual void endLatch(LibertyGroup *group);
  virtual void beginLatchBank(LibertyGroup *group);
  virtual void endLatchBank(LibertyGroup *group);
  virtual void beginSequential(LibertyGroup *group,
			       bool is_register,
			       bool is_bank);
  virtual void seqPortNames(LibertyGroup *group,
			    const char *&out_name,
			    const char *&out_inv_name,
			    bool &has_size,
			    int &size);
  virtual void checkLatchEnableSense(FuncExpr *enable_func,
				     int line);
  virtual void visitClockedOn(LibertyAttr *attr);
  virtual void visitDataIn(LibertyAttr *attr);
  virtual void visitClear(LibertyAttr *attr);
  virtual void visitPreset(LibertyAttr *attr);
  virtual void visitClrPresetVar1(LibertyAttr *attr);
  virtual void visitClrPresetVar2(LibertyAttr *attr);

  virtual void beginStatetable(LibertyGroup *group);
  virtual void endStatetable(LibertyGroup *group);
  virtual void visitTable(LibertyAttr *attr);

  virtual void beginTiming(LibertyGroup *group);
  virtual void endTiming(LibertyGroup *group);
  virtual void visitRelatedPin(LibertyAttr *attr);
  virtual void visitRelatedPin(LibertyAttr *attr,
			       RelatedPortGroup *group);
  virtual void visitRelatedBusPins(LibertyAttr *attr);
  virtual void visitRelatedBusPins(LibertyAttr *attr,
				   RelatedPortGroup *group);
  virtual void visitRelatedOutputPin(LibertyAttr *attr);
  virtual void visitTimingType(LibertyAttr *attr);
  virtual void visitTimingSense(LibertyAttr *attr);
  virtual void visitSdfCondStart(LibertyAttr *attr);
  virtual void visitSdfCondEnd(LibertyAttr *attr);
  virtual void visitMode(LibertyAttr *attr);
  virtual void visitIntrinsicRise(LibertyAttr *attr);
  virtual void visitIntrinsicFall(LibertyAttr *attr);
  virtual void visitIntrinsic(LibertyAttr *attr,
			      const RiseFall *rf);
  virtual void visitRiseResistance(LibertyAttr *attr);
  virtual void visitFallResistance(LibertyAttr *attr);
  virtual void visitRiseFallResistance(LibertyAttr *attr,
				       const RiseFall *rf);
  virtual void visitValue(LibertyAttr *attr);
  virtual void visitValues(LibertyAttr *attr);
  virtual void beginCellRise(LibertyGroup *group);
  virtual void beginCellFall(LibertyGroup *group);
  virtual void endCellRiseFall(LibertyGroup *group);
  virtual void beginRiseTransition(LibertyGroup *group);
  virtual void endRiseFallTransition(LibertyGroup *group);
  virtual void beginFallTransition(LibertyGroup *group);
  virtual void beginRiseConstraint(LibertyGroup *group);
  virtual void endRiseFallConstraint(LibertyGroup *group);
  virtual void beginFallConstraint(LibertyGroup *group);

  virtual void beginRiseTransitionDegredation(LibertyGroup *group);
  virtual void beginFallTransitionDegredation(LibertyGroup *group);
  virtual void endRiseFallTransitionDegredation(LibertyGroup *group);

  virtual void beginTableModel(LibertyGroup *group,
			       TableTemplateType type,
			       const RiseFall *rf,
			       float scale,
			       ScaleFactorType scale_factor_type);
  virtual void endTableModel();
  virtual void beginTimingTableModel(LibertyGroup *group,
				     const RiseFall *rf,
				     ScaleFactorType scale_factor_type);
  virtual void beginTable(LibertyGroup *group,
			  TableTemplateType type,
			  float scale);
  virtual void endTable();
  virtual void makeTable(LibertyAttr *attr,
			 float scale);
  virtual FloatTable *makeFloatTable(LibertyAttr *attr,
				     size_t rows,
				     size_t cols,
				     float scale);

  virtual void beginLut(LibertyGroup *group);
  virtual void endLut(LibertyGroup *group);

  virtual void beginTestCell(LibertyGroup *group);
  virtual void endTestCell(LibertyGroup *group);

  virtual void beginModeDef(LibertyGroup *group);
  virtual void endModeDef(LibertyGroup *group);
  virtual void beginModeValue(LibertyGroup *group);
  virtual void endModeValue(LibertyGroup *group);
  virtual void visitWhen(LibertyAttr *attr);
  virtual void visitSdfCond(LibertyAttr *attr);

  // Power attributes.
  virtual void beginTableTemplatePower(LibertyGroup *group);
  virtual void beginLeakagePower(LibertyGroup *group);
  virtual void endLeakagePower(LibertyGroup *group);
  virtual void beginInternalPower(LibertyGroup *group);
  virtual void endInternalPower(LibertyGroup *group);
  virtual InternalPowerGroup *makeInternalPowerGroup(int line);
  virtual void beginFallPower(LibertyGroup *group);
  virtual void beginRisePower(LibertyGroup *group);
  virtual void endRiseFallPower(LibertyGroup *group);
  virtual void endPower(LibertyGroup *group);
  virtual void visitRelatedGroundPin(LibertyAttr *attr);
  virtual void visitRelatedPowerPin(LibertyAttr *attr);
  virtual void visitRelatedPgPin(LibertyAttr *attr);
  virtual void makeInternalPowers(LibertyPort *port,
				  InternalPowerGroup *power_group);
  virtual void makeInternalPowers(LibertyPort *port,
				  const char *related_port_name,
				  PortNameBitIterator &related_port_iter,
				  InternalPowerGroup *power_group);

  // AOCV attributes.
  virtual void beginTableTemplateOcv(LibertyGroup *group);
  virtual void visitOcvArcDepth(LibertyAttr *attr);
  virtual void visitDefaultOcvDerateGroup(LibertyAttr *attr);
  virtual void visitOcvDerateGroup(LibertyAttr *attr);
  virtual void beginOcvDerate(LibertyGroup *group);
  virtual void endOcvDerate(LibertyGroup *group);
  virtual void beginOcvDerateFactors(LibertyGroup *group);
  virtual void endOcvDerateFactors(LibertyGroup *group);
  virtual void visitRfType(LibertyAttr *attr);
  virtual void visitDerateType(LibertyAttr *attr);
  virtual void visitPathType(LibertyAttr *attr);

  // POCV attributes.
  virtual void beginOcvSigmaCellRise(LibertyGroup *group);
  virtual void beginOcvSigmaCellFall(LibertyGroup *group);
  virtual void endOcvSigmaCell(LibertyGroup *group);
  virtual void beginOcvSigmaRiseTransition(LibertyGroup *group);
  virtual void beginOcvSigmaFallTransition(LibertyGroup *group);
  virtual void endOcvSigmaTransition(LibertyGroup *group);
  virtual void beginOcvSigmaRiseConstraint(LibertyGroup *group);
  virtual void beginOcvSigmaFallConstraint(LibertyGroup *group);
  virtual void endOcvSigmaConstraint(LibertyGroup *group);
  virtual void visitSigmaType(LibertyAttr *attr);

  // PgPin group.
  virtual void beginPgPin(LibertyGroup *group);
  virtual void endPgPin(LibertyGroup *group);
  virtual void visitPgType(LibertyAttr *attr);
  virtual void visitVoltageName(LibertyAttr *attr);

  // ccs receiver capacitance
  virtual void beginReceiverCapacitance(LibertyGroup *group);
  virtual void endReceiverCapacitance(LibertyGroup *group);

  virtual void visitSegement(LibertyAttr *attr);

  virtual void beginReceiverCapacitance1Rise(LibertyGroup *group);
  virtual void endReceiverCapacitanceRiseFall(LibertyGroup *group);
  virtual void beginReceiverCapacitance1Fall(LibertyGroup *group);
  virtual void beginReceiverCapacitance2Rise(LibertyGroup *group);
  virtual void beginReceiverCapacitance2Fall(LibertyGroup *group);
  void beginReceiverCapacitance(LibertyGroup *group,
                                int index,
                                const RiseFall *rf);
  void endReceiverCapacitance(LibertyGroup *group,
                              int index,
                              const RiseFall *rf);
  // ccs
  void beginOutputCurrentRise(LibertyGroup *group);
  void beginOutputCurrentFall(LibertyGroup *group);
  void beginOutputCurrent(const RiseFall *rf,
                          LibertyGroup *group);
  void endOutputCurrentRiseFall(LibertyGroup *group);
  void beginVector(LibertyGroup *group);
  void endVector(LibertyGroup *group);
  void visitReferenceTime(LibertyAttr *attr);

  void beginNormalizedDriverWaveform(LibertyGroup *group);
  void endNormalizedDriverWaveform(LibertyGroup *group);
  void visitDriverWaveformName(LibertyAttr *attr);

  void visitDriverWaveformRise(LibertyAttr *attr);
  void visitDriverWaveformFall(LibertyAttr *attr);
  void visitDriverWaveformRiseFall(LibertyAttr *attr,
                                   const RiseFall *rf);

  void beginCcsn(LibertyGroup *group);
  void endCcsn(LibertyGroup *group);
  void beginEcsmWaveform(LibertyGroup *group);
  void endEcsmWaveform(LibertyGroup *group);
  LibertyPort *findPort(LibertyCell *cell,
			const char *port_name);

protected:
  LibertyPort *makePort(LibertyCell *cell,
                        const char *port_name);
  LibertyPort *makeBusPort(LibertyCell *cell,
                           const char *bus_name,
                           int from_index,
                           int to_index,
                           BusDcl *bus_dcl);

  TimingModel *makeScalarCheckModel(float value,
                                    ScaleFactorType scale_factor_type,
                                    const RiseFall *rf);
  void makeMinPulseWidthArcs(LibertyPort *port,
                             int line);
  void setEnergyScale();
  void defineVisitors();
  virtual void begin(LibertyGroup *group);
  virtual void end(LibertyGroup *group);
  void defineGroupVisitor(const char *type,
			  LibraryGroupVisitor begin_visitor,
			  LibraryGroupVisitor end_visitor);
  void defineAttrVisitor(const char *attr_name,
			 LibraryAttrVisitor visitor);
  void parseNames(const char *name_str);
  void clearAxisValues();
  void makeTableAxis(int index,
                     LibertyAttr *attr);

  StringSeq *parseNameList(const char *name_list);
  StdStringSeq parseTokenList(const char *token_str,
                              const char separator);
  LibertyPort *findPort(const char *port_name);
  float defaultCap(LibertyPort *port);
  virtual void visitVariable(LibertyVariable *var);
  void visitPorts(std::function<void (LibertyPort *port)> func);
  StateInputValues parseStateInputValues(StdStringSeq &inputs,
                                         LibertyAttr *attr);
  StateInternalValues parseStateInternalValues(StdStringSeq &states,
                                               LibertyAttr *attr);

  const char *getAttrString(LibertyAttr *attr);
  void getAttrInt(LibertyAttr *attr,
		  // Return values.
		  int &value,
		  bool &exists);
  void getAttrFloat(LibertyAttr *attr,
		    // Return values.
		    float &value,
		    bool &valid);
  void getAttrFloat(LibertyAttr *attr,
		    LibertyAttrValue *attr_value,
		    // Return values.
		    float &value,
		    bool &valid);
  void getAttrFloat2(LibertyAttr *attr,
		     // Return values.
		     float &value1,
		     float &value2,
		     bool &exists);
  void parseStringFloatList(const char *float_list,
			    float scale,
			    FloatSeq *values,
			    LibertyAttr *attr);
  LogicValue getAttrLogicValue(LibertyAttr *attr);
  void getAttrBool(LibertyAttr *attr,
		   // Return values.
		   bool &value,
		   bool &exists);
  void visitVariable(int index,
		     LibertyAttr *attr);
  void visitIndex(int index,
		  LibertyAttr *attr);
  TableAxisPtr makeAxis(int index,
                        LibertyGroup *group);
  FloatSeq *readFloatSeq(LibertyAttr *attr,
			 float scale);
  void variableValue(const char *var,
		     float &value,
		     bool &exists);
  FuncExpr *parseFunc(const char *func,
		      const char *attr_name,
		      int line);
  void libWarn(int id,
               LibertyStmt *stmt,
	       const char *fmt,
	       ...)
    __attribute__((format (printf, 4, 5)));
  void libWarn(int id,
               int line,
	       const char *fmt,
	       ...)
    __attribute__((format (printf, 4, 5)));
  void libError(int id,
                LibertyStmt *stmt,
		const char *fmt, ...)
    __attribute__((format (printf, 4, 5)));

  const char *filename_;
  bool infer_latches_;
  Report *report_;
  Debug *debug_;
  Network *network_;
  LibertyBuilder builder_;
  LibertyVariableMap *var_map_;
  LibertyLibrary *library_;
  LibraryGroupMap group_begin_map_;
  LibraryGroupMap group_end_map_;
  LibraryAttrMap attr_visitor_map_;
  Wireload *wireload_;
  WireloadSelection *wireload_selection_;
  const char *default_wireload_;
  const char *default_wireload_selection_;
  ScaleFactors *scale_factors_;
  ScaleFactors *save_scale_factors_;
  bool have_input_threshold_[RiseFall::index_count];
  bool have_output_threshold_[RiseFall::index_count];
  bool have_slew_lower_threshold_[RiseFall::index_count];
  bool have_slew_upper_threshold_[RiseFall::index_count];
  TableTemplate *tbl_template_;
  LibertyCell *cell_;
  LibertyCell *scaled_cell_owner_;
  const char *ocv_derate_name_;
  PortGroupSeq cell_port_groups_;
  OperatingConditions *op_cond_;
  LibertyPortSeq *ports_;
  PortGroup *port_group_;
  LibertyPortSeq *saved_ports_;
  PortGroup *saved_port_group_;
  StringSeq bus_names_;
  bool in_bus_;
  bool in_bundle_;
  bool in_ccsn_;
  bool in_ecsm_waveform_;
  TableAxisVariable axis_var_[3];
  FloatSeq *axis_values_[3];
  int type_bit_from_;
  bool type_bit_from_exists_;
  int type_bit_to_;
  bool type_bit_to_exists_;
  SequentialGroup *sequential_;
  SequentialGroupSeq cell_sequentials_;
  StatetableGroup *statetable_;
  TimingGroup *timing_;
  InternalPowerGroup *internal_power_;
  LeakagePowerGroup *leakage_power_;
  LeakagePowerGroupSeq leakage_powers_;
  const RiseFall *rf_;
  int index_;
  OcvDerate *ocv_derate_;
  const RiseFallBoth *rf_type_;
  const EarlyLateAll *derate_type_;
  const EarlyLateAll *sigma_type_;
  PathType path_type_;
  LibertyPgPort *pg_port_;
  ScaleFactorType scale_factor_type_;
  TableAxisPtr axis_[3];
  TablePtr table_;
  float table_model_scale_;
  ModeDef *mode_def_;
  ModeValueDef *mode_value_;
  LibertyFuncSeq cell_funcs_;
  float time_scale_;
  float cap_scale_;
  float res_scale_;
  float volt_scale_;
  float current_scale_;
  float power_scale_;
  float energy_scale_;
  float distance_scale_;
  const char *default_operating_condition_;
  ReceiverModelPtr receiver_model_;
  OutputWaveformSeq output_currents_;
  OutputWaveforms *output_waveforms_;
  float reference_time_;
  bool reference_time_exists_;
  std::string driver_waveform_name_;

  TestCell *test_cell_;
  // Saved state while parsing test_cell.
  LibertyCell *save_cell_;
  PortGroupSeq save_cell_port_groups_;
  StatetableGroup *save_statetable_;
  SequentialGroupSeq save_cell_sequentials_;
  LibertyFuncSeq save_cell_funcs_;

  static constexpr char escape_ = '\\';

private:
  friend class PortNameBitIterator;
  friend class TimingGroup;
};

// Reference to a function that will be parsed at the end of the cell
// definition when all of the ports are defined.
class LibertyFunc
{
public:
  LibertyFunc(const char *expr,
              LibertySetFunc set_func,
	      bool invert,
	      const char *attr_name,
	      int line);
  ~LibertyFunc();
  const char *expr() const { return expr_; }
  LibertySetFunc setFunc() const { return set_func_; }
  bool invert() const { return invert_; }
  const char *attrName() const { return attr_name_; }
  int line() const { return line_; }

protected:
  const char *expr_;
  LibertySetFunc set_func_;
  bool invert_;
  const char *attr_name_;
  int line_;
};

// Port attributes that refer to other ports cannot be parsed
// until all of the ports are defined.  This class saves them
// so they can be parsed at the end of the cell.
class PortGroup
{
public:
  PortGroup(LibertyPortSeq *ports,
	    int line);
  ~PortGroup();
  LibertyPortSeq *ports() const { return ports_; }
  TimingGroupSeq &timingGroups() { return timings_; }
  void addTimingGroup(TimingGroup *timing);
  InternalPowerGroupSeq &internalPowerGroups() { return internal_power_groups_; }
  void addInternalPowerGroup(InternalPowerGroup *internal_power);
  ReceiverModel *receiverModel() const { return receiver_model_; }
  void setReceiverModel(ReceiverModelPtr receiver_model);
  int line() const { return line_; }

private:
  LibertyPortSeq *ports_;
  TimingGroupSeq timings_;
  InternalPowerGroupSeq internal_power_groups_;
  ReceiverModel *receiver_model_;
  int line_;
};

// Liberty group with related_pins group attribute.
class RelatedPortGroup
{
public:
  explicit RelatedPortGroup(int line);
  virtual ~RelatedPortGroup();
  int line() const { return line_; }
  StringSeq *relatedPortNames() const { return related_port_names_; }
  void setRelatedPortNames(StringSeq *names);
  bool isOneToOne() const { return is_one_to_one_; }
  void setIsOneToOne(bool one);

protected:
  StringSeq *related_port_names_;
  bool is_one_to_one_;
  int line_;
};

class SequentialGroup
{
public:
  SequentialGroup(bool is_register,
		  bool is_bank,
		  LibertyPort *out_port,
		  LibertyPort *out_inv_port,
		  int size,
		  int line);
  ~SequentialGroup();
  LibertyPort *outPort() const { return out_port_; }
  LibertyPort *outInvPort() const { return out_inv_port_; }
  int size() const { return size_; }
  bool isRegister() const { return is_register_; }
  bool isBank() const { return is_bank_; }
  const char *clock() const { return clk_; }
  void setClock(const char *clk);
  const char *data() const { return data_; }
  void setData(const char *data);
  const char *clear() const { return clear_; }
  void setClear(const char *clr);
  const char *preset() const { return preset_; }
  void setPreset(const char *preset);
  LogicValue clrPresetVar1() const { return clr_preset_var1_; }
  void setClrPresetVar1(LogicValue var);
  LogicValue clrPresetVar2() const { return clr_preset_var2_; }
  void setClrPresetVar2(LogicValue var);
  int line() const { return line_; }

protected:
  bool is_register_;
  bool is_bank_;
  LibertyPort *out_port_;
  LibertyPort *out_inv_port_;
  int size_;
  const char *clk_;
  const char *data_;
  const char *preset_;
  const char *clear_;
  LogicValue clr_preset_var1_;
  LogicValue clr_preset_var2_;
  int line_;
};

class StatetableGroup
{
public:
  StatetableGroup(StdStringSeq &input_ports,
                  StdStringSeq &internal_ports,
                  int line);
  const StdStringSeq &inputPorts() const { return input_ports_; }
  const StdStringSeq &internalPorts() const { return internal_ports_; }
  void addRow(StateInputValues &input_values,
              StateInternalValues &current_values,
              StateInternalValues &next_values);
  StatetableRows &table() { return table_; }
  int line() const { return line_; }

private:
  StdStringSeq input_ports_;
  StdStringSeq internal_ports_;
  StatetableRows table_;
  int line_;
};

class TimingGroup : public RelatedPortGroup
{
public:
  explicit TimingGroup(int line);
  virtual ~TimingGroup();
  TimingArcAttrsPtr attrs() { return attrs_; }
  const char *relatedOutputPortName()const {return related_output_port_name_;}
  void setRelatedOutputPortName(const char *name);
  void intrinsic(const RiseFall *rf,
		 // Return values.
		 float &value,
		 bool &exists);
  void setIntrinsic(const RiseFall *rf,
		    float value);
  void resistance(const RiseFall *rf,
		  // Return values.
		  float &value,
		  bool &exists);
  void setResistance(const RiseFall *rf,
		     float value);
  TableModel *cell(const RiseFall *rf);
  void setCell(const RiseFall *rf,
	       TableModel *model);
  TableModel *constraint(const RiseFall *rf);
  void setConstraint(const RiseFall *rf,
		     TableModel *model);
  TableModel *transition(const RiseFall *rf);
  void setTransition(const RiseFall *rf,
		     TableModel *model);
  void makeTimingModels(LibertyCell *cell,
			LibertyReader *visitor);
  void setDelaySigma(const RiseFall *rf,
		     const EarlyLate *early_late,
		     TableModel *model);
  void setSlewSigma(const RiseFall *rf,
		    const EarlyLate *early_late,
		    TableModel *model);
  void setConstraintSigma(const RiseFall *rf,
			  const EarlyLate *early_late,
			  TableModel *model);
  void setReceiverModel(ReceiverModelPtr receiver_model);
  OutputWaveforms *outputWaveforms(const RiseFall *rf);
  void setOutputWaveforms(const RiseFall *rf,
                        OutputWaveforms *output_current);
  
protected:
  void makeLinearModels(LibertyCell *cell);
  void makeTableModels(LibertyCell *cell,
                       LibertyReader *reader);

  TimingArcAttrsPtr attrs_;
  const char *related_output_port_name_;
  float intrinsic_[RiseFall::index_count];
  bool intrinsic_exists_[RiseFall::index_count];
  float resistance_[RiseFall::index_count];
  bool resistance_exists_[RiseFall::index_count];
  TableModel *cell_[RiseFall::index_count];
  TableModel *constraint_[RiseFall::index_count];
  TableModel *constraint_sigma_[RiseFall::index_count][EarlyLate::index_count];
  TableModel *transition_[RiseFall::index_count];
  TableModel *delay_sigma_[RiseFall::index_count][EarlyLate::index_count];
  TableModel *slew_sigma_[RiseFall::index_count][EarlyLate::index_count];
  OutputWaveforms *output_waveforms_[RiseFall::index_count];
  ReceiverModelPtr receiver_model_;
};

class InternalPowerGroup : public InternalPowerAttrs, public RelatedPortGroup
{
public:
  explicit InternalPowerGroup(int line);
  virtual ~InternalPowerGroup();
};

class LeakagePowerGroup : public LeakagePowerAttrs
{
public:
  explicit LeakagePowerGroup(int line);
  virtual ~LeakagePowerGroup();

protected:
  int line_;
};

// Named port iterator.  Port name can be:
//   Single bit port name - iterates over port.
//   Bus port name - iterates over bus bit ports.
//   Bus range - iterates over bus bit ports.
class PortNameBitIterator : public Iterator<LibertyPort*>
{
public:
  PortNameBitIterator(LibertyCell *cell,
		      const char *port_name,
		      LibertyReader *visitor,
		      int line);
  ~PortNameBitIterator();
  virtual bool hasNext();
  virtual LibertyPort *next();
  unsigned size() const { return size_; }

protected:
  void findRangeBusNameNext();

  void init(const char *port_name);
  LibertyCell *cell_;
  LibertyReader *visitor_;
  int line_;
  LibertyPort *port_;
  LibertyPortMemberIterator *bit_iterator_;
  LibertyPort *range_bus_port_;
  std::string range_bus_name_;
  LibertyPort *range_name_next_;
  int range_from_;
  int range_to_;
  int range_bit_;
  unsigned size_;
};

class OutputWaveform
{
public:
  OutputWaveform(float axis_value1,
                 float axis_value2,
                 Table1 *currents,
                 float reference_time);
  ~OutputWaveform();
  float slew() const { return slew_; }
  float cap() const { return cap_; }
  Table1 *currents() const { return currents_; }
  Table1 *stealCurrents();
  float referenceTime() { return reference_time_; }

private:
  float slew_;
  float cap_;
  Table1 *currents_;
  float reference_time_;
};

} // namespace
