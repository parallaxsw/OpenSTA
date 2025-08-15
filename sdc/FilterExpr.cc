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

FilterError::FilterError(std::string_view error)  :
  Exception()
{
  error_ = error;
}

const char *
FilterError::what() const noexcept
{
  return error_.c_str();
}

FilterExpr::Token::Token(std::string text, FilterExpr::PredicateToken::Kind kind): text(text), kind(kind) {}

FilterExpr::PredicateToken::PredicateToken(std::string property, std::string op, std::string arg): Token(
    property + " " + op + " " + arg,
    FilterExpr::PredicateToken::Kind::predicate
), property(property), op(op), arg(arg) {}

FilterExpr::FilterExpr(std::string expression): raw_(expression) {}

std::vector<std::shared_ptr<FilterExpr::Token>> FilterExpr::postfix(bool sta_boolean_props_as_int) {
    auto infix = lex(sta_boolean_props_as_int);
    return shuntingYard(infix);
}

std::vector<std::shared_ptr<FilterExpr::Token>> FilterExpr::lex(bool sta_boolean_props_as_int) {
    std::vector<std::pair<std::regex, FilterExpr::Token::Kind>> token_regexes = {
        {std::regex("^\\s+"), FilterExpr::Token::Kind::skip},
        {std::regex("^defined\\(([a-zA-Z_]+)\\)"), FilterExpr::Token::Kind::defined},
        {std::regex("^undefined\\(([a-zA-Z_]+)\\)"), FilterExpr::Token::Kind::undefined},
        {std::regex("^@?([a-zA-Z_]+) *((==|!=|=~|!~) *([0-9a-zA-Z_\\/$\\[\\]*?]+))?"), FilterExpr::Token::Kind::predicate},
        {std::regex("^(&&)"), FilterExpr::Token::Kind::op_and},
        {std::regex("^(\\|\\|)"), FilterExpr::Token::Kind::op_or},
        {std::regex("^(!)"), FilterExpr::Token::Kind::op_inv},
        {std::regex("^(\\()"), FilterExpr::Token::Kind::op_lparen},
        {std::regex("^(\\))"), FilterExpr::Token::Kind::op_rparen},
    };
    
    std::vector<std::shared_ptr<FilterExpr::Token>> result;
    const char* ptr = &raw_[0];
    bool match = false;
    while (*ptr != '\0') {
        match = false;
        for (auto& [regex, kind]: token_regexes) {
            std::cmatch token_match;
            if (std::regex_search(ptr, token_match, regex)) {
                if (kind == FilterExpr::Token::Kind::predicate) {
                    std::string property = token_match[1].str();
                    
                    // The default operation on a predicate if an op and arg are
                    // omitted is == 1/== true.
                    std::string op = "==";
                    std::string arg = (sta_boolean_props_as_int ? "1" : "true");
                    
                    if (token_match[2].length() != 0) {
                        op = token_match[3].str();
                        arg = token_match[4].str();
                    }
                    result.push_back(std::make_shared<PredicateToken>(property, op, arg));
                } else if (kind == FilterExpr::Token::Kind::defined) {
                    result.push_back(std::make_shared<Token>(token_match[1].str(), kind));
                } else if (kind == FilterExpr::Token::Kind::undefined) {
                    result.push_back(std::make_shared<Token>(token_match[1].str(), kind));
                } else if (kind != FilterExpr::Token::Kind::skip) {
                    result.push_back(std::make_shared<Token>(std::string(ptr, token_match.length()), kind));
                }
                ptr += token_match.length();
                match = true;
                break;
            };
        }
        if (!match) {
            throw FilterError(std::string("unexpected character starting at: '") + ptr + "'");
        }
    }
    return result;
}

std::vector<std::shared_ptr<FilterExpr::Token>> FilterExpr::shuntingYard(std::vector<std::shared_ptr<FilterExpr::Token>>& infix) {
    std::vector<std::shared_ptr<FilterExpr::Token>> output;
    std::stack<std::shared_ptr<FilterExpr::Token>> operator_stack;
    
    for (auto &pToken: infix) {
        switch (pToken->kind) {
        case FilterExpr::Token::Kind::predicate:
            output.push_back(pToken);
            break;
        case FilterExpr::Token::Kind::op_or:
            [[fallthrough]];
        case FilterExpr::Token::Kind::op_and:
            // The operators' enum values are ascending by precedence:
            // inv > and > or
            while (operator_stack.size() && operator_stack.top()->kind > pToken->kind) {
                output.push_back(operator_stack.top());
                operator_stack.pop();
            }
            operator_stack.push(pToken);
            break;
        case FilterExpr::Token::Kind::op_inv:
            // Unary with highest precedence, no need for the while loop
            operator_stack.push(pToken);
            break;
        case FilterExpr::Token::Kind::defined:
            operator_stack.push(pToken);
            break;
        case FilterExpr::Token::Kind::undefined:
            operator_stack.push(pToken);
            break;
        case FilterExpr::Token::Kind::op_lparen:
            operator_stack.push(pToken);
            break;
        case FilterExpr::Token::Kind::op_rparen:
            if (operator_stack.empty()) {
                throw FilterError("extraneous ) in expression");
            }
            while (operator_stack.size() && operator_stack.top()->kind != FilterExpr::Token::Kind::op_lparen) {
                output.push_back(operator_stack.top());
                operator_stack.pop();   
                if (operator_stack.empty()) {
                    throw FilterError("extraneous ) in expression");
                }
            }
            // guaranteed to be lparen at this point
            operator_stack.pop();
            break;
        default:
            // unhandled/skip
            break;
        }
    }
    
    while (operator_stack.size()) {
        if (operator_stack.top()->kind == FilterExpr::Token::Kind::op_lparen) {
            throw FilterError("unmatched ( in expression");
        }
        output.push_back(operator_stack.top());
        operator_stack.pop();
    }
    
    return output;
}
