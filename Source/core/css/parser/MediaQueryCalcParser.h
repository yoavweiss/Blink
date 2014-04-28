// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaQueryCalcParser_h
#define MediaQueryCalcParser_h

#include "core/css/parser/MediaQueryToken.h"

namespace WebCore {

class MediaQueryCalcParser {

public:
    static int parse(MediaQueryTokenIterator start, MediaQueryTokenIterator end);

private:
    MediaQueryCalcParser() {}
    bool calcToReversePolishNotation(MediaQueryTokenIterator start, MediaQueryTokenIterator end);
    int calculate();
    bool handleOperator(Vector<MediaQueryToken>& stack, const MediaQueryToken& token);

    Vector<MediaQueryToken> m_reversePolishNotationTokenList;
};

} // namespace WebCore

#endif // MediaQueryCalcParser_h

