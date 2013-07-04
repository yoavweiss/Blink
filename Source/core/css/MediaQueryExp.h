/*
 * CSS Media Query
 *
 * Copyright (C) 2006 Kimmo Kinnunen <kimmo.t.kinnunen@nokia.com>.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MediaQueryExp_h
#define MediaQueryExp_h

#include "core/css/CSSValue.h"
#include "core/css/MediaFeatureNames.h"
#include "wtf/PassOwnPtr.h"
#include "wtf/RefPtr.h"

namespace WebCore {
class CSSParserValueList;

class MediaQueryExp {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<MediaQueryExp> create(const String& mediaFeature, CSSParserValueList*);
    ~MediaQueryExp();

    String mediaFeature() const { return m_mediaFeature; }

    CSSValue* value() const { return m_value.get(); }

    bool operator==(const MediaQueryExp& other) const
    {
        return (other.m_mediaFeature == m_mediaFeature)
            && ((!other.m_value && !m_value)
                || (other.m_value && m_value && other.m_value->equals(*m_value)));
    }

    bool isViewportDependent() const;

    String serialize() const;

    PassOwnPtr<MediaQueryExp> copy() const { return adoptPtr(new MediaQueryExp(*this)); }

    void reportMemoryUsage(MemoryObjectInfo*) const;

private:
    MediaQueryExp(const String& mediaFeature, PassRefPtr<CSSValue>);

    String m_mediaFeature;
    RefPtr<CSSValue> m_value;
};

} // namespace

#endif
