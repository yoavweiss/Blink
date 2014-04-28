// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/css/parser/MediaQueryCalcParser.h"

#include "core/css/MediaValues.h"
#include "core/css/parser/MediaQueryToken.h"

namespace WebCore {

int MediaQueryCalcParser::parse(MediaQueryTokenIterator start, MediaQueryTokenIterator end, unsigned fontSize, unsigned viewportWidth, unsigned viewportHeight)
{
    MediaQueryCalcParser parser(fontSize, viewportWidth, viewportHeight);
    int result = 0;
    if (parser.calcToReversePolishNotation(start, end))
        parser.calculate(result);
    return result;
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
    printf("handling operator %c\n",token.delimiter());
    bool stackOperatorPriority;
    bool incomingOperatorPriority;

    if (!operatorPriority(token.delimiter(), incomingOperatorPriority))
        return false;
    if (!stack.isEmpty() && stack.last().type() == DelimiterToken) {
        if (!operatorPriority(stack.last().delimiter(), stackOperatorPriority))
            return false;
        if (!incomingOperatorPriority || stackOperatorPriority) {
            appendOperator(stack.last());
            stack.removeLast();
        }
    }
    stack.append(token);
    return true;
}

bool MediaQueryCalcParser::appendNumber(const MediaQueryToken& token)
{
    MediaQueryCalcValue value;
    if (token.type() == PercentageToken) {
        value.value = token.numericValue() / 100.0;
    } else if(token.type() == NumberToken) {
        value.value = token.numericValue();
    } else {
        int result = 0;
        if (!MediaValues::computeLength(token.numericValue(), token.unitType(), m_fontSize, m_viewportWidth, m_viewportHeight, result))
            return false;
        value.value = result;
    }
    printf("appending number %f %f\n", token.numericValue(), value.value);
    m_valueList.append(value);
    return true;
}

void MediaQueryCalcParser::appendOperator(const MediaQueryToken& token)
{
    printf("appending operator\n");
    MediaQueryCalcValue value;
    value.operation = token.delimiter();
    m_valueList.append(value);
}

bool MediaQueryCalcParser::calcToReversePolishNotation(MediaQueryTokenIterator start, MediaQueryTokenIterator end)
{
    // This method implements the shunting yard algorithm, to turn the calc syntax into a reverse polish notation.
    // http://en.wikipedia.org/wiki/Shunting-yard_algorithm

    Vector<MediaQueryToken> stack;
    for (MediaQueryTokenIterator it = start; it != end; ++it) {
        MediaQueryTokenType type = it->type();
        printf("Got Token %d\n", type);
        switch (type) {
        case NumberToken:
        case PercentageToken:
        case DimensionToken:
            // If the token is a number, then add it to the output queue.
            if (!appendNumber(*it)){
                printf("returning false\n");
                return false;
            }
            break;
        case DelimiterToken:
            printf("Got delimiter\n");
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
            printf("found right parens %d\n", (int)stack.size());
            while (!stack.isEmpty() && stack.last().type() != LeftParenthesisToken && stack.last().type() != FunctionToken) {
                appendOperator(stack.last());
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
        appendOperator(stack.last());
        stack.removeLast();
    }
    return true;
}

static bool operateOnStack(Vector<double>& stack, UChar operation)
{
    printf("Got an operator %d\n", (int)stack.size());
    if (stack.size() < 2)
        return false;
    double rightOperand = stack.last();
    stack.removeLast();
    double leftOperand = stack.last();
    stack.removeLast();
    switch (operation) {
    case '+':
        stack.append(rightOperand + leftOperand);
        break;
    case '-':
        stack.append(rightOperand - leftOperand);
        break;
    case '*':
        stack.append(rightOperand * leftOperand);
        break;
    case '/':
        stack.append(rightOperand / leftOperand);
        break;
    default:
        return false;
    }
    return true;
}

bool MediaQueryCalcParser::calculate(int& result)
{
    Vector<double> stack;
    for (Vector<MediaQueryCalcValue>::iterator it = m_valueList.begin(); it != m_valueList.end(); ++it) {
        if (it->operation == 0) {
            printf("value %f\n", it->value);
            stack.append(it->value);
        } else {
            printf("operation %c\n", it->operation);
            if (!operateOnStack(stack, it->operation))
                return false;
        }
    }
    if (stack.size() == 1) {
        result = stack.last();
        return true;
    }
    return false;
}

} // namespace WebCore
