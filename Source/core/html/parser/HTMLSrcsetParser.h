// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HTMLSrcsetParser_h
#define HTMLSrcsetParser_h

#include "wtf/text/WTFString.h"

namespace WebCore {

class ImageCandidate {
public:
    ImageCandidate()
    {
    }

    ImageCandidate& operator=(const ImageCandidate& candidate)
    {
        m_string = candidate.m_string;
        m_scaleFactor = candidate.m_scaleFactor;
        return *this; 
    }

    ImageCandidate(const String& source, unsigned start, unsigned length, float scaleFactor)
        //: m_string(source.createView(start, length))
        : m_string(StringView(source.impl(), start, length))
        , m_scaleFactor(scaleFactor)
    {
    }

    inline String toString() const
    {
        if (m_string.is8Bit())
            return String(m_string.characters8(), m_string.length());
        else
            return String(m_string.characters16(), m_string.length());
    }

    inline float scaleFactor() const
    {
        return m_scaleFactor;
    }

    inline bool isEmpty() const
    {
        return m_string.isEmpty();
    }

private:
    StringView m_string;
    float m_scaleFactor;
};

ImageCandidate bestFitSourceForSrcsetAttribute(float deviceScaleFactor, const String& srcsetAttribute);

String bestFitSourceForImageAttributes(float deviceScaleFactor, const String& srcAttribute, const String& srcsetAttribute);

String bestFitSourceForImageAttributes(float deviceScaleFactor, const String& srcAttribute, ImageCandidate& srcsetImageCandidate);

}

#endif
