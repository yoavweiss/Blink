// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/css/parser/MediaQueryCalcParser.h"

#include "core/css/parser/MediaQueryToken.h"

namespace WebCore {

int MediaQueryCalcParser::parse(MediaQueryTokenIterator start, MediaQueryTokenIterator end)
{
    MediaQueryCalcParser parser;
    parser.calcToReversePolishNotation(start, end);
    return parser.calculate();
}

static bool operatorPriority(UChar cc, bool& highPriority)
{
    if (cc == '+' || cc == '-')
        highPriority = false;
    else if (cc == '*' || cc == '/')
        highPriority = true;
    else
        return false;
    return true;
}

bool MediaQueryCalcParser::handleOperator(Vector<MediaQueryToken>& stack, const MediaQueryToken& token)
{
    // If the token is an operator, o1, then:
    // while there is an operator token, o2, at the top of the stack, and
    // either o1 is left-associative and its precedence is equal to that of o2,
    // or o1 has precedence less than that of o2,
    // pop o2 off the stack, onto the output queue;
    // push o1 onto the stack.
    bool stackOperatorPriority;
    bool incomingOperatorPriority;

    if (!operatorPriority(token.delimiter(), incomingOperatorPriority))
        return false;
    if (!stack.isEmpty() && stack.last().type() == DelimiterToken) {
        if (!operatorPriority(stack.last().delimiter(), stackOperatorPriority))
            return false;
        if (!incomingOperatorPriority || stackOperatorPriority) {
            m_reversePolishNotationTokenList.append(stack.last());
            stack.removeLast();
        }
    }
    stack.append(token);
    return true;
}

bool MediaQueryCalcParser::calcToReversePolishNotation(MediaQueryTokenIterator start, MediaQueryTokenIterator end)
{
    // This method implements the shunting yard algorithm, to turn the calc syntax into a reverse polish notation.
    // http://en.wikipedia.org/wiki/Shunting-yard_algorithm

    Vector<MediaQueryToken> stack;
    for (MediaQueryTokenIterator it = start; it != end; ++it) {
        MediaQueryTokenType type = it->type();
        switch (type) {
        case NumberToken:
        case PercentageToken:
        case DimensionToken:
            // If the token is a number, then add it to the output queue.
            m_reversePolishNotationTokenList.append(*it);
            break;
        case DelimiterToken:
            if (!handleOperator(stack, *it))
                return false;
            break;
        case FunctionToken:
            if (it->value() != "calc")
                return false;
            // "calc(" is the same as "("
        case LeftParenthesisToken:
            // If the token is a left parenthesis, then push it onto the stack.
            stack.append(*it);
            break;
        case RightParenthesisToken:
            // If the token is a right parenthesis:
            // Until the token at the top of the stack is a left parenthesis, pop operators off the stack onto the output queue.
            while (!stack.isEmpty() && (stack.last().type() == LeftParenthesisToken || stack.last().type() == FunctionToken)) {
                m_reversePolishNotationTokenList.append(stack.last());
                stack.removeLast();
            }
            // If the stack runs out without finding a left parenthesis, then there are mismatched parentheses.
            if (stack.isEmpty())
                return false;
            // Pop the left parenthesis from the stack, but not onto the output queue.
            stack.removeLast();
            break;
        case CommentToken:
        case WhitespaceToken:
        case EOFToken:
            break;
        case IdentToken:
        case CommaToken:
        case ColonToken:
        case SemicolonToken:
        case LeftBraceToken:
        case LeftBracketToken:
        case RightBraceToken:
        case RightBracketToken:
        case StringToken:
        case BadStringToken:
            return false;
        }
    }

    // When there are no more tokens to read:
    // While there are still operator tokens in the stack:
    while (!stack.isEmpty()) {
        // If the operator token on the top of the stack is a parenthesis, then there are mismatched parentheses.
        MediaQueryTokenType type = stack.last().type();
        if (type == LeftParenthesisToken || type == FunctionToken)
            return false;
        // Pop the operator onto the output queue.
        m_reversePolishNotationTokenList.append(stack.last());
        stack.removeLast();
    }
    return true;
}

} // namespace WebCore
