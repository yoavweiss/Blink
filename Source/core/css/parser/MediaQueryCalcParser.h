// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaQueryCalcParser_h
#define MediaQueryCalcParser_h

#include "core/css/parser/MediaQueryToken.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

struct MediaQueryCalcValue {
    double value;
    UChar operation;

    MediaQueryCalcValue()
        : value(0)
        , operation(0)
    {
    }
};

class MediaQueryCalcParser {

public:
    static int parse(MediaQueryTokenIterator start, MediaQueryTokenIterator end, unsigned fontSize, unsigned viewportWidth, unsigned viewportHeight);

private:
    MediaQueryCalcParser(unsigned fontSize, unsigned viewportWidth, unsigned viewportHeight)
        : m_fontSize(fontSize)
        , m_viewportWidth(viewportWidth)
        , m_viewportHeight(viewportHeight)
    {
    }

    bool calcToReversePolishNotation(MediaQueryTokenIterator start, MediaQueryTokenIterator end);
    bool calculate(int& result);
    bool appendNumber(const MediaQueryToken&);
    bool handleOperator(Vector<MediaQueryToken>& stack, const MediaQueryToken&);
    void appendOperator(const MediaQueryToken&);

    Vector<MediaQueryCalcValue> m_valueList;
    unsigned m_fontSize;
    unsigned m_viewportWidth;
    unsigned m_viewportHeight;
};

} // namespace WebCore

#endif // MediaQueryCalcParser_h

