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

#include "config.h"
#include "core/html/parser/HTMLResourcePreloader.h"

#include "core/dom/Document.h"
#include "core/fetch/FetchInitiatorInfo.h"
#include "core/fetch/ResourceFetcher.h"
#include "core/html/HTMLImport.h"
#include "core/css/MediaList.h"
#include "core/css/MediaQueryEvaluator.h"
#include "core/platform/HistogramSupport.h"
#include "core/rendering/RenderObject.h"

namespace WebCore {

bool PreloadRequest::isSafeToSendToAnotherThread() const
{
    return m_initiatorName.isSafeToSendToAnotherThread()
        && m_charset.isSafeToSendToAnotherThread()
        && m_resourceURL.isSafeToSendToAnotherThread()
        && m_mediaAttribute.isSafeToSendToAnotherThread()
        && m_baseURL.isSafeToSendToAnotherThread();
}

KURL PreloadRequest::completeURL(Document* document)
{
    return document->completeURL(m_resourceURL, m_baseURL.isEmpty() ? document->url() : m_baseURL);
}

FetchRequest PreloadRequest::resourceRequest(Document* document)
{
    ASSERT(isMainThread());
    FetchInitiatorInfo initiatorInfo;
    initiatorInfo.name = m_initiatorName;
    initiatorInfo.position = m_initiatorPosition;
    FetchRequest request(ResourceRequest(completeURL(document)), initiatorInfo);

    // FIXME: It's possible CORS should work for other request types?
    if (m_resourceType == Resource::Script)
        request.mutableResourceRequest().setAllowCookies(m_crossOriginModeAllowsCookies);
    return request;
}

void HTMLResourcePreloader::takeAndPreload(PreloadRequestStream& r)
{
    PreloadRequestStream requests;
    requests.swap(r);

    for (PreloadRequestStream::iterator it = requests.begin(); it != requests.end(); ++it)
        preload(it->release());
}

static bool mediaAttributeMatches(Frame* frame, RenderStyle* renderStyle, const String& attributeValue)
{
    RefPtr<MediaQuerySet> mediaQueries = MediaQuerySet::create(attributeValue);
    MediaQueryEvaluator mediaQueryEvaluator("screen", frame, renderStyle);
    return mediaQueryEvaluator.eval(mediaQueries.get());
}

void HTMLResourcePreloader::preload(PassOwnPtr<PreloadRequest> preload)
{
    Document* executingDocument = m_document->import() ? m_document->import()->master() : m_document;
    Document* loadingDocument = m_document;

    if(preload->bundleStart()){
        m_inBundle = true;
        return;
    }
    else if(preload->bundleEnd()){
        m_inBundle = false;
        m_foundBundleResource = false;
        return;
    }

    ASSERT(executingDocument->frame());
    ASSERT(executingDocument->renderer());
    ASSERT(executingDocument->renderer()->style());
    if (!preload->media().isEmpty() && !mediaAttributeMatches(executingDocument->frame(), executingDocument->renderer()->style(), preload->media()))
        return;

    if(m_inBundle && m_foundBundleResource)
        return;

    m_foundBundleResource = true;

    FetchRequest request = preload->resourceRequest(m_document);
    HistogramSupport::histogramCustomCounts("WebCore.PreloadDelayMs", static_cast<int>(1000 * (monotonicallyIncreasingTime() - preload->discoveryTime())), 0, 2000, 20);
    loadingDocument->fetcher()->preload(preload->resourceType(), request, preload->charset());
}


}
