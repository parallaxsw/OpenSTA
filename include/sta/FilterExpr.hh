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
#include <string>
#include "StringSeq.hh"
#include "Error.hh"

namespace sta {

using std::string;

class FilterSyntaxError : public Exception
{
public:
  explicit FilterSyntaxError(const char* error);
  virtual ~FilterSyntaxError() noexcept {}
  virtual const char *what() const noexcept;

private:
  std::string error_;
};

class FilterUnexpectedCharacter : public Exception
{
public:
  explicit FilterUnexpectedCharacter(const char* error);
  virtual ~FilterUnexpectedCharacter() noexcept {}
  virtual const char *what() const noexcept;

private:
  std::string error_;
};

class FilterExpr {
public:
    struct Token {
        enum class Kind {
            skip = 0,
            predicate,
            op_lparen,
            op_rparen,
            op_or,
            op_and,
            op_inv
        };
        std::string text;
        Kind kind;
    };
    
    FilterExpr(std::string expression);
    
    std::vector<std::string> postfix(bool sta_boolean_props_as_int);
private:
    std::vector<Token> lex(bool sta_boolean_props_as_int);
    std::vector<Token> shuntingYard(const std::vector<Token>& infix);
    
    std::string raw_;
};

} // namespace
