// OpenSTA, Static Timing Analyzer
// Copyright (c) 2026, Parallax Software, Inc.
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

#include "PatternMatch.hh"

#include <cctype>
#include <tcl.h>

#include "Sta.hh"
#include "Variables.hh"

namespace sta {

static std::string stripEscapedBus(std::string_view str);

// TCL variable sta_case_insensitive_matching.
// When enabled, all glob/regexp name matching done by the get_*/find_*_matching
// commands (ports, instances, nets, cells, clocks, ...) ignores case.
// Read lazily so that toggling the variable takes effect immediately and so
// that PatternMatch objects constructed before Sta exists are safe.
static bool
caseInsensitiveMatching()
{
  const Sta *sta = Sta::sta();
  return sta
    && sta->variables()
    && sta->variables()->caseInsensitiveMatching();
}

PatternMatch::PatternMatch(std::string_view pattern,
                           bool is_regexp,
                           bool nocase,
                           Tcl_Interp *interp) :
  pattern_(pattern),
  is_regexp_(is_regexp),
  nocase_(nocase),
  interp_(interp),
  regexp_(nullptr)
{
  if (is_regexp_)
    compileRegexp();
}

PatternMatch::PatternMatch(std::string_view pattern) :
  pattern_(pattern),
  is_regexp_(false),
  nocase_(false),
  interp_(nullptr),
  regexp_(nullptr)
{
}

PatternMatch::PatternMatch(std::string_view pattern,
                           const PatternMatch *inherit_from) :
  pattern_(pattern),
  is_regexp_(inherit_from->is_regexp_),
  nocase_(inherit_from->nocase_),
  interp_(inherit_from->interp_),
  regexp_(nullptr)
{
  if (is_regexp_)
    compileRegexp();
}

void
PatternMatch::compileRegexp()
{
  int flags = TCL_REG_ADVANCED;
  if (nocase_ || caseInsensitiveMatching())
    flags |= TCL_REG_NOCASE;
  std::string anchored_pattern;
  anchored_pattern += '^';
  anchored_pattern += pattern_;
  anchored_pattern += '$';
  Tcl_Obj *pattern_obj = Tcl_NewStringObj(anchored_pattern.c_str(),
                                          anchored_pattern.size());
  Tcl_IncrRefCount(pattern_obj);
  regexp_ = Tcl_GetRegExpFromObj(interp_, pattern_obj, flags);
  Tcl_DecrRefCount(pattern_obj);
  if (regexp_ == nullptr && interp_)
    throw RegexpCompileError(pattern_);
}

static bool
regexpWildcards(std::string_view pattern)
{
  return pattern.find_first_of(".+*?[]") != std::string_view::npos;
}

bool
PatternMatch::hasWildcards() const
{
  // When case-insensitive matching is enabled the find_*_matching functions
  // must iterate and match each candidate name rather than doing an exact
  // (case-sensitive) hashed lookup, so report the pattern as having wildcards.
  if (caseInsensitiveMatching())
    return true;
  if (is_regexp_)
    return regexpWildcards(pattern_);
  else
    return patternWildcards(pattern_);
}

bool
PatternMatch::match(std::string_view str) const
{
  if (regexp_) {
    // regexp_ was compiled with TCL_REG_NOCASE when case-insensitive
    // matching is enabled.
    std::string buf(str);
    const char *cstr = buf.c_str();
    return Tcl_RegExpExec(nullptr, regexp_, cstr, cstr) == 1;
  }
  bool nocase = caseInsensitiveMatching();
  return patternMatchNoCase(pattern_, str, nocase)  ||
    (Sta::sta()->stripEscapedBus() &&
          patternMatchNoCase(pattern_, stripEscapedBus(str), nocase));
}

std::string
stripEscapedBus(std::string_view str)
{
  // strip trailing escaped bus indices from str
  // bus\[8\] -> bus
  // bus\[8\]\[7\] -> bus
  // bus\[8\]\[7\]\[6\] -> bus
  // bus\[8\].hello -> bus\[8\].hello
  // bus\[hello\].world -> bus\[hello\].world
  // etc.
  std::string result{str};
  while (true) {
    int len = static_cast<int>(result.size());
    // Minimum pattern \[0\] is 5 chars
    if (len < 5)
      break;
    // Must end with \]
    if (result[len - 1] != ']' || result[len - 2] != '\\')
      break;
    int pos = len - 3;
    // Need at least one digit
    if (pos < 0 || !isdigit(static_cast<unsigned char>(result[pos])))
      break;
    // Skip digits
    while (pos >= 0 && isdigit(static_cast<unsigned char>(result[pos])))
      pos--;
    // Must have \[ at current position
    if (pos < 1 || result[pos] != '[' || result[pos - 1] != '\\')
      break;
    // Truncate before the \[
    result.resize(pos - 1);
  }
  return result;
}

bool
PatternMatch::matchNoCase(std::string_view str) const
{
  if (regexp_) {
    std::string buf(str);
    const char *cstr = buf.c_str();
    return Tcl_RegExpExec(nullptr, regexp_, cstr, cstr) == 1;
  }
  bool nocase = nocase_ || caseInsensitiveMatching();
  return patternMatchNoCase(pattern_, str, nocase) ||
    (Sta::sta()->stripEscapedBus() &&
          patternMatchNoCase(pattern_, stripEscapedBus(str), nocase));
}

////////////////////////////////////////////////////////////////

RegexpCompileError::RegexpCompileError(std::string_view pattern)
{
  error_ = "TCL failed to compile regular expression '";
  error_.append(pattern.data(), pattern.size());
  error_ += "'.";
}

const char *
RegexpCompileError::what() const noexcept
{
  return error_.c_str();
}

////////////////////////////////////////////////////////////////

bool
patternMatch(std::string_view pattern,
             std::string_view str)
{
  size_t pi = 0;
  size_t si = 0;
  while (pi < pattern.size() && si < str.size()
         && (str[si] == pattern[pi] || pattern[pi] == '?')) {
    pi++;
    si++;
  }
  if (pi == pattern.size() && si == str.size())
    return true;
  if (pi < pattern.size() && pattern[pi] == '*') {
    if (pi + 1 == pattern.size())
      return true;
    while (si < str.size()) {
      if (patternMatch(pattern.substr(pi + 1), str.substr(si)))
        return true;
      si++;
    }
  }
  return false;
}

static bool
equalCase(char s,
          char p,
          bool nocase)
{
  return nocase
    ? std::tolower(static_cast<unsigned char>(s))
        == std::tolower(static_cast<unsigned char>(p))
    : s == p;
}

bool
patternMatchNoCase(std::string_view pattern,
                   std::string_view str,
                   bool nocase)
{
  size_t pi = 0;
  size_t si = 0;
  while (pi < pattern.size() && si < str.size()
         && (equalCase(str[si], pattern[pi], nocase) || pattern[pi] == '?')) {
    pi++;
    si++;
  }
  if (pi == pattern.size() && si == str.size())
    return true;
  if (pi < pattern.size() && pattern[pi] == '*') {
    if (pi + 1 == pattern.size())
      return true;
    while (si < str.size()) {
      if (patternMatchNoCase(pattern.substr(pi + 1), str.substr(si), nocase))
        return true;
      si++;
    }
  }
  return false;
}

bool
patternWildcards(std::string_view pattern)
{
  return pattern.find_first_of("*?") != std::string_view::npos;
}

} // namespace sta
