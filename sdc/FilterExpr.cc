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

#include "Error.hh"
#include "FilterExpr.hh"

#include <regex>
#include <stack>
#include <functional>

using namespace sta;

FilterSyntaxError::FilterSyntaxError(const char *what)  :
  Exception()
{
  error_ = what;
}

const char *
FilterSyntaxError::what() const noexcept
{
  return error_.c_str();
}

FilterUnexpectedCharacter::FilterUnexpectedCharacter(const char *starting_at)  :
  Exception()
{
  error_ = "unexpected character starting at: '";
  error_ += starting_at;
  error_ += "'";
}

const char *
FilterUnexpectedCharacter::what() const noexcept
{
  return error_.c_str();
}

FilterExpr::FilterExpr(std::string expression): raw_(expression) {}

std::vector<std::string> FilterExpr::postfix(bool sta_boolean_props_as_int) {
    auto infix = lex(sta_boolean_props_as_int);
    auto postfix = shuntingYard(infix);
    std::vector<std::string> result;
    for (auto& token: postfix) {
        result.push_back(token.text);
    }
    return result;
}

std::vector<FilterExpr::Token> FilterExpr::lex(bool sta_boolean_props_as_int) {
    std::vector<std::pair<std::regex, FilterExpr::Token::Kind>> token_regexes = {
        {std::regex("^\\s+"), FilterExpr::Token::Kind::skip},
        {std::regex("^@?([a-zA-Z_]+) *((==|!=|=~|!~) *([0-9a-zA-Z_\\/$\\[\\]*]+))?"), FilterExpr::Token::Kind::predicate},
        {std::regex("^(&&)"), FilterExpr::Token::Kind::op_and},
        {std::regex("^(\\|\\|)"), FilterExpr::Token::Kind::op_or},
        {std::regex("^(!)"), FilterExpr::Token::Kind::op_inv},
        {std::regex("^(\\()"), FilterExpr::Token::Kind::op_lparen},
        {std::regex("^(\\))"), FilterExpr::Token::Kind::op_rparen},
    };
    
    std::vector<FilterExpr::Token> result;
    const char* ptr = &raw_[0];
    bool match = false;
    while (*ptr != 0) {
        match = false;
        for (auto& [regex, kind]: token_regexes) {
            std::cmatch token_match;
            if (std::regex_search(ptr, token_match, regex)) {
                if (kind == FilterExpr::Token::Kind::predicate) {
                    std::string final_predicate;
                    if (token_match[2].length() == 0 && token_match[3].length() == 0) { // empty final match
                        final_predicate = token_match[1].str() + " == " + (sta_boolean_props_as_int ? "1" : "true");
                    } else {
                        final_predicate = token_match[1].str() + " " + token_match[3].str() + " " + token_match[4].str();
                    }
                    auto token = Token { final_predicate, kind };
                    result.push_back(token);
                } else if (kind != FilterExpr::Token::Kind::skip) {
                    auto token = Token { std::string(ptr, token_match.length()), kind };
                    result.push_back(token);
                }
                ptr += token_match.length();
                match = true;
                break;
            };
        }
        if (!match) {
            throw FilterUnexpectedCharacter(ptr);
        }
    }
    return result;
}

std::vector<FilterExpr::Token> FilterExpr::shuntingYard(const std::vector<Token>& infix) {
    std::vector<FilterExpr::Token> output;
    std::stack<FilterExpr::Token> operator_stack;
    
    for (auto& token: infix) {
        switch (token.kind) {
        case FilterExpr::Token::Kind::predicate:
            output.push_back(token);
            break;
        case FilterExpr::Token::Kind::op_or:
        case FilterExpr::Token::Kind::op_and:
            while (operator_stack.size() && operator_stack.top().kind > token.kind) {
                output.push_back(operator_stack.top());
                operator_stack.pop();
            }
        case FilterExpr::Token::Kind::op_inv:
        case FilterExpr::Token::Kind::op_lparen:
            operator_stack.push(token);
            break;
        case FilterExpr::Token::Kind::op_rparen:
            if (operator_stack.empty()) {
                throw FilterSyntaxError("extraneous ) in expression");
            }
            while (operator_stack.size() && operator_stack.top().kind != FilterExpr::Token::Kind::op_lparen) {
                output.push_back(operator_stack.top());
                operator_stack.pop();   
                if (operator_stack.empty()) {
                    throw FilterSyntaxError("extraneous ) in expression");
                }
            }
            // guaranteed to be lparen at this point
            operator_stack.pop();
            break;
        }
    }
    
    while (operator_stack.size()) {
        if (operator_stack.top().kind == FilterExpr::Token::Kind::op_lparen) {
            throw FilterSyntaxError("unmatched ( in expression");
        }
        output.push_back(operator_stack.top());
        operator_stack.pop();
    }
    
    return output;
}
