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
        {"calc(500px+10em)", 660},
        {0, 0} // Do not remove the terminator line.
    };


    for (unsigned i = 0; testCases[i].input; ++i) {
        Vector<MediaQueryToken> tokens;
        MediaQueryTokenizer::tokenize(testCases[i].input, tokens);
        int output = MediaQueryCalcParser::parse(tokens.begin(), tokens.end());
        ASSERT_EQ(testCases[i].output, output);
    }
}

} // namespace
