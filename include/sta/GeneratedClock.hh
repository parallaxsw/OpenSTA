#pragma once

#include "LibertyClass.hh"
#include "SdcClass.hh"

namespace sta {

class GeneratedClock
{
public:
  ~GeneratedClock();
  const char *name() const { return name_; }
  const char *clockPin() const { return clock_pin_; }
  const char *masterPin() const { return master_pin_; }
  int dividedBy() const { return divided_by_; }
  int multipliedBy() const { return multiplied_by_; }
  float dutyCycle() const { return duty_cycle_; }
  bool invert() const { return invert_; }
  IntSeq *edges() const { return edges_; }
  FloatSeq *edgeShifts() const { return edge_shifts_; }

protected:
  GeneratedClock(const char *name,
                 const char *clock_pin,
                 const char *master_pin,
                 int divided_by,
                 int multiplied_by,
                 float duty_cycle,
                 bool invert,
                 IntSeq *edges,
                 FloatSeq *edge_shifts);

  const char *name_;
  const char *clock_pin_;
  const char *master_pin_;
  int divided_by_;
  int multiplied_by_;
  float duty_cycle_;
  bool invert_;
  IntSeq *edges_;
  FloatSeq *edge_shifts_;

  friend class LibertyCell;
};

} // namespace
