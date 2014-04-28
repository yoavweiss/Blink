// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "core/css/parser/MediaQueryCalcParser.h"

#include "core/css/parser/MediaQueryTokenizer.h"

#include <gtest/gtest.h>

namespace WebCore {

typedef struct {
    const char* input;
    const int output;
} TestCase;

TEST(MediaQueryCalcParserTest, Basic)
{
    // The first string represents the input string.
    // The second string represents the output string, if present.
    // Otherwise, the output string is identical to the first string.
    TestCase testCases[] = {
        {"calc(500px + 10em)", 660},
        {"calc(500px + 2 * 10em)", 820},
        {"calc(500px + 2*10em)", 820},
        {"calc(500px + 50%*10em)", 580},
        {"calc(500px + (50%*10em + 13px))", 593},
        {"calc(100vw + (50%*10em + 13px))", 593},
        {"calc(100vh + (50%*10em + 13px))", 736},
        {0, 0} // Do not remove the terminator line.
    };


    for (unsigned i = 0; testCases[i].input; ++i) {
        printf("test %s\n", testCases[i].input);
        Vector<MediaQueryToken> tokens;
        MediaQueryTokenizer::tokenize(testCases[i].input, tokens);
        int output = MediaQueryCalcParser::parse(tokens.begin(), tokens.end(), 16, 500, 643);
        ASSERT_EQ(testCases[i].output, output);
    }
}

} // namespace
