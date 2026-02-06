#include "GeneratedClock.hh"

#include "Liberty.hh"
#include "StringUtil.hh"

namespace sta {

GeneratedClock::GeneratedClock(const char *name,
                                const char *clock_pin,
                                const char *master_pin,
                                int divided_by,
                                int multiplied_by,
                                float duty_cycle,
                                bool invert,
                                IntSeq *edges,
                                FloatSeq *edge_shifts) :
  name_(name ? stringCopy(name) : nullptr),
  clock_pin_(clock_pin ? stringCopy(clock_pin) : nullptr),
  master_pin_(master_pin ? stringCopy(master_pin) : nullptr),
  divided_by_(divided_by),
  multiplied_by_(multiplied_by),
  duty_cycle_(duty_cycle),
  invert_(invert),
  edges_(edges),
  edge_shifts_(edge_shifts)
{
}

GeneratedClock::~GeneratedClock()
{
  if (name_)
    stringDelete(name_);
  if (clock_pin_)
    stringDelete(clock_pin_);
  if (master_pin_)
    stringDelete(master_pin_);
  if (edges_)
    delete edges_;
  if (edge_shifts_)
    delete edge_shifts_;
}

} // namespace
