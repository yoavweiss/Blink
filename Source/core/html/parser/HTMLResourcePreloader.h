/*
 * Copyright (C) 2013 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTMLResourcePreloader_h
#define HTMLResourcePreloader_h

#include "core/fetch/FetchRequest.h"
#include "core/fetch/Resource.h"
#include "wtf/CurrentTime.h"
#include "wtf/text/TextPosition.h"

namespace WebCore {

class PreloadRequest {
public:
    static PassOwnPtr<PreloadRequest> create(const String& initiatorName, const TextPosition& initiatorPosition, const String& resourceURL, const KURL& baseURL, Resource::Type resourceType, const String& mediaAttribute)
    {
        return adoptPtr(new PreloadRequest(initiatorName, initiatorPosition, resourceURL, baseURL, resourceType, mediaAttribute, false, false));
    }

    static PassOwnPtr<PreloadRequest> create(const String& initiatorName, const TextPosition& initiatorPosition, const String& resourceURL, const KURL& baseURL, Resource::Type resourceType)
    {
        return adoptPtr(new PreloadRequest(initiatorName, initiatorPosition, resourceURL, baseURL, resourceType, "", false, false));
    }

    static PassOwnPtr<PreloadRequest> create(bool bundleStart, bool bundleEnd)
    {
        return adoptPtr(new PreloadRequest(String(), TextPosition(), String(), KURL(), Resource::Raw, String(), bundleStart, bundleEnd));
    }

    bool isSafeToSendToAnotherThread() const;

    FetchRequest resourceRequest(Document*);

    const String& charset() const { return m_charset; }
    const String& media() const { return m_mediaAttribute; }
    double discoveryTime() const { return m_discoveryTime; }
    void setCharset(const String& charset) { m_charset = charset.isolatedCopy(); }
    void setCrossOriginModeAllowsCookies(bool allowsCookies) { m_crossOriginModeAllowsCookies = allowsCookies; }
    Resource::Type resourceType() const { return m_resourceType; }
    const String& resourceURL() const { return m_resourceURL; }
    bool bundleStart() { return m_bundleStart; }
    bool bundleEnd() { return m_bundleEnd; }

private:
    PreloadRequest(const String& initiatorName, 
        const TextPosition& initiatorPosition, 
        const String& resourceURL, 
        const KURL& baseURL, 
        Resource::Type resourceType, 
        const String& mediaAttribute,
        bool bundleStart,
        bool bundleEnd)
        : m_initiatorName(initiatorName.isolatedCopy())
        , m_initiatorPosition(initiatorPosition)
        , m_resourceURL(resourceURL.isolatedCopy())
        , m_baseURL(baseURL.copy())
        , m_resourceType(resourceType)
        , m_mediaAttribute(mediaAttribute.isolatedCopy())
        , m_crossOriginModeAllowsCookies(false)
        , m_discoveryTime(monotonicallyIncreasingTime())
        , m_bundleStart(bundleStart)
        , m_bundleEnd(bundleEnd)
    {
    }

    KURL completeURL(Document*);

    String m_initiatorName;
    TextPosition m_initiatorPosition;
    String m_resourceURL;
    KURL m_baseURL;
    String m_charset;
    Resource::Type m_resourceType;
    String m_mediaAttribute;
    bool m_crossOriginModeAllowsCookies;
    double m_discoveryTime;
    bool m_bundleStart;
    bool m_bundleEnd;
};

typedef Vector<OwnPtr<PreloadRequest> > PreloadRequestStream;

class HTMLResourcePreloader {
    WTF_MAKE_NONCOPYABLE(HTMLResourcePreloader); WTF_MAKE_FAST_ALLOCATED;
public:
    explicit HTMLResourcePreloader(Document* document)
        : m_document(document)
        , m_weakFactory(this)
        , m_inBundle(false)
        , m_foundBundleResource(false)
    {
    }

    void takeAndPreload(PreloadRequestStream&);
    void preload(PassOwnPtr<PreloadRequest>);

    WeakPtr<HTMLResourcePreloader> createWeakPtr() { return m_weakFactory.createWeakPtr(); }

private:
    Document* m_document;
    WeakPtrFactory<HTMLResourcePreloader> m_weakFactory;
    bool m_inBundle;
    bool m_foundBundleResource;
};

}

#endif
