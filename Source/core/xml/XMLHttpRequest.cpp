/*
 *  Copyright (C) 2004, 2006, 2008 Apple Inc. All rights reserved.
 *  Copyright (C) 2005-2007 Alexey Proskuryakov <ap@webkit.org>
 *  Copyright (C) 2007, 2008 Julien Chaffraix <jchaffraix@webkit.org>
 *  Copyright (C) 2008, 2011 Google Inc. All rights reserved.
 *  Copyright (C) 2012 Intel Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "core/xml/XMLHttpRequest.h"

#include "bindings/core/v8/ExceptionState.h"
#include "core/FetchInitiatorTypeNames.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/DOMException.h"
#include "core/dom/DOMImplementation.h"
#include "core/dom/DocumentParser.h"
#include "core/dom/ExceptionCode.h"
#include "core/dom/XMLDocument.h"
#include "core/editing/markup.h"
#include "core/events/Event.h"
#include "core/fetch/FetchUtils.h"
#include "core/fileapi/Blob.h"
#include "core/fileapi/File.h"
#include "core/frame/Settings.h"
#include "core/frame/UseCounter.h"
#include "core/frame/csp/ContentSecurityPolicy.h"
#include "core/html/DOMFormData.h"
#include "core/html/HTMLDocument.h"
#include "core/html/parser/TextResourceDecoder.h"
#include "core/inspector/ConsoleMessage.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "core/inspector/InspectorTraceEvents.h"
#include "core/loader/ThreadableLoader.h"
#include "core/streams/ReadableStream.h"
#include "core/streams/ReadableStreamImpl.h"
#include "core/streams/Stream.h"
#include "core/streams/UnderlyingSource.h"
#include "core/xml/XMLHttpRequestProgressEvent.h"
#include "core/xml/XMLHttpRequestUpload.h"
#include "platform/Logging.h"
#include "platform/RuntimeEnabledFeatures.h"
#include "platform/SharedBuffer.h"
#include "platform/blob/BlobData.h"
#include "platform/network/HTTPParsers.h"
#include "platform/network/ParsedContentType.h"
#include "platform/network/ResourceError.h"
#include "platform/network/ResourceRequest.h"
#include "public/platform/WebURLRequest.h"
#include "wtf/ArrayBuffer.h"
#include "wtf/ArrayBufferView.h"
#include "wtf/Assertions.h"
#include "wtf/RefCountedLeakCounter.h"
#include "wtf/StdLibExtras.h"
#include "wtf/text/CString.h"

namespace blink {

DEFINE_DEBUG_ONLY_GLOBAL(WTF::RefCountedLeakCounter, xmlHttpRequestCounter, ("XMLHttpRequest"));

static bool isSetCookieHeader(const AtomicString& name)
{
    return equalIgnoringCase(name, "set-cookie") || equalIgnoringCase(name, "set-cookie2");
}

static void replaceCharsetInMediaType(String& mediaType, const String& charsetValue)
{
    unsigned pos = 0, len = 0;

    findCharsetInMediaType(mediaType, pos, len);

    if (!len) {
        // When no charset found, do nothing.
        return;
    }

    // Found at least one existing charset, replace all occurrences with new charset.
    while (len) {
        mediaType.replace(pos, len, charsetValue);
        unsigned start = pos + charsetValue.length();
        findCharsetInMediaType(mediaType, pos, len, start);
    }
}

static void logConsoleError(ExecutionContext* context, const String& message)
{
    if (!context)
        return;
    // FIXME: It's not good to report the bad usage without indicating what source line it came from.
    // We should pass additional parameters so we can tell the console where the mistake occurred.
    context->addConsoleMessage(ConsoleMessage::create(JSMessageSource, ErrorMessageLevel, message));
}

namespace {

class ReadableStreamSource : public GarbageCollectedFinalized<ReadableStreamSource>, public UnderlyingSource {
    USING_GARBAGE_COLLECTED_MIXIN(ReadableStreamSource);
public:
    ReadableStreamSource(XMLHttpRequest* owner) : m_owner(owner) { }
    virtual ~ReadableStreamSource() { }
    virtual void pullSource() OVERRIDE { }
    virtual ScriptPromise cancelSource(ScriptState* scriptState, ScriptValue reason) OVERRIDE
    {
        m_owner->abort();
        return ScriptPromise::cast(scriptState, v8::Undefined(scriptState->isolate()));
    }
    virtual void trace(Visitor* visitor) OVERRIDE
    {
        visitor->trace(m_owner);
        UnderlyingSource::trace(visitor);
    }

private:
    // This is RawPtr in non-oilpan build to avoid the reference cycle. To
    // avoid use-after free, the associated ReadableStream must be closed
    // or errored when m_owner is gone.
    RawPtrWillBeMember<XMLHttpRequest> m_owner;
};

} // namespace

PassRefPtrWillBeRawPtr<XMLHttpRequest> XMLHttpRequest::create(ExecutionContext* context, PassRefPtr<SecurityOrigin> securityOrigin)
{
    RefPtrWillBeRawPtr<XMLHttpRequest> xmlHttpRequest = adoptRefWillBeNoop(new XMLHttpRequest(context, securityOrigin));
    xmlHttpRequest->suspendIfNeeded();

    return xmlHttpRequest.release();
}

XMLHttpRequest::XMLHttpRequest(ExecutionContext* context, PassRefPtr<SecurityOrigin> securityOrigin)
    : ActiveDOMObject(context)
    , m_timeoutMilliseconds(0)
    , m_loaderIdentifier(0)
    , m_state(UNSENT)
    , m_lengthDownloadedToFile(0)
    , m_receivedLength(0)
    , m_lastSendLineNumber(0)
    , m_exceptionCode(0)
    , m_progressEventThrottle(this)
    , m_responseTypeCode(ResponseTypeDefault)
    , m_securityOrigin(securityOrigin)
    , m_async(true)
    , m_includeCredentials(false)
    , m_parsedResponse(false)
    , m_error(false)
    , m_uploadEventsAllowed(true)
    , m_uploadComplete(false)
    , m_sameOriginRequest(true)
    , m_downloadingToFile(false)
{
#ifndef NDEBUG
    xmlHttpRequestCounter.increment();
#endif
}

XMLHttpRequest::~XMLHttpRequest()
{
#ifndef NDEBUG
    xmlHttpRequestCounter.decrement();
#endif
}

Document* XMLHttpRequest::document() const
{
    ASSERT(executionContext()->isDocument());
    return toDocument(executionContext());
}

SecurityOrigin* XMLHttpRequest::securityOrigin() const
{
    return m_securityOrigin ? m_securityOrigin.get() : executionContext()->securityOrigin();
}

XMLHttpRequest::State XMLHttpRequest::readyState() const
{
    return m_state;
}

ScriptString XMLHttpRequest::responseText(ExceptionState& exceptionState)
{
    if (m_responseTypeCode != ResponseTypeDefault && m_responseTypeCode != ResponseTypeText) {
        exceptionState.throwDOMException(InvalidStateError, "The value is only accessible if the object's 'responseType' is '' or 'text' (was '" + responseType() + "').");
        return ScriptString();
    }
    if (m_error || (m_state != LOADING && m_state != DONE))
        return ScriptString();
    return m_responseText;
}

ScriptString XMLHttpRequest::responseJSONSource()
{
    ASSERT(m_responseTypeCode == ResponseTypeJSON);

    if (m_error || m_state != DONE)
        return ScriptString();
    return m_responseText;
}

void XMLHttpRequest::initResponseDocument()
{
    // The W3C spec requires the final MIME type to be some valid XML type, or text/html.
    // If it is text/html, then the responseType of "document" must have been supplied explicitly.
    bool isHTML = responseIsHTML();
    if ((m_response.isHTTP() && !responseIsXML() && !isHTML)
        || (isHTML && m_responseTypeCode == ResponseTypeDefault)
        || executionContext()->isWorkerGlobalScope()) {
        m_responseDocument = nullptr;
        return;
    }

    DocumentInit init = DocumentInit::fromContext(document()->contextDocument(), m_url);
    if (isHTML)
        m_responseDocument = HTMLDocument::create(init);
    else
        m_responseDocument = XMLDocument::create(init);

    // FIXME: Set Last-Modified.
    m_responseDocument->setSecurityOrigin(securityOrigin());
    m_responseDocument->setContextFeatures(document()->contextFeatures());
    m_responseDocument->setMimeType(finalResponseMIMETypeWithFallback());
}

Document* XMLHttpRequest::responseXML(ExceptionState& exceptionState)
{
    if (m_responseTypeCode != ResponseTypeDefault && m_responseTypeCode != ResponseTypeDocument) {
        exceptionState.throwDOMException(InvalidStateError, "The value is only accessible if the object's 'responseType' is '' or 'document' (was '" + responseType() + "').");
        return 0;
    }

    if (m_error || m_state != DONE)
        return 0;

    if (!m_parsedResponse) {
        initResponseDocument();
        if (!m_responseDocument)
            return nullptr;

        m_responseDocument->setContent(m_responseText.flattenToString());
        if (!m_responseDocument->wellFormed())
            m_responseDocument = nullptr;

        m_parsedResponse = true;
    }

    return m_responseDocument.get();
}

Blob* XMLHttpRequest::responseBlob()
{
    ASSERT(m_responseTypeCode == ResponseTypeBlob);

    // We always return null before DONE.
    if (m_error || m_state != DONE)
        return 0;

    if (!m_responseBlob) {
        OwnPtr<BlobData> blobData = BlobData::create();
        if (m_downloadingToFile) {
            ASSERT(!m_binaryResponseBuilder.get());

            // When responseType is set to "blob", we redirect the downloaded
            // data to a file-handle directly in the browser process. We get
            // the file-path from the ResourceResponse directly instead of
            // copying the bytes between the browser and the renderer.
            String filePath = m_response.downloadedFilePath();
            // If we errored out or got no data, we still return a blob, just
            // an empty one.
            if (!filePath.isEmpty() && m_lengthDownloadedToFile) {
                blobData->appendFile(filePath);
                // FIXME: finalResponseMIMETypeWithFallback() defaults to
                // text/xml which may be incorrect. Replace it with
                // finalResponseMIMEType() after compatibility investigation.
                blobData->setContentType(finalResponseMIMETypeWithFallback());
            }
            m_responseBlob = Blob::create(BlobDataHandle::create(blobData.release(), m_lengthDownloadedToFile));
        } else {
            size_t size = 0;
            if (m_binaryResponseBuilder.get() && m_binaryResponseBuilder->size()) {
                size = m_binaryResponseBuilder->size();
                blobData->appendBytes(m_binaryResponseBuilder->data(), size);
                blobData->setContentType(finalResponseMIMETypeWithFallback());
                m_binaryResponseBuilder.clear();
            }
            m_responseBlob = Blob::create(BlobDataHandle::create(blobData.release(), size));
        }
    }

    return m_responseBlob.get();
}

ArrayBuffer* XMLHttpRequest::responseArrayBuffer()
{
    ASSERT(m_responseTypeCode == ResponseTypeArrayBuffer);

    if (m_error || m_state != DONE)
        return 0;

    if (!m_responseArrayBuffer.get()) {
        if (m_binaryResponseBuilder.get() && m_binaryResponseBuilder->size() > 0) {
            m_responseArrayBuffer = m_binaryResponseBuilder->getAsArrayBuffer();
            if (!m_responseArrayBuffer) {
                // m_binaryResponseBuilder failed to allocate an ArrayBuffer.
                // We need to crash the renderer since there's no way defined in
                // the spec to tell this to the user.
                CRASH();
            }
            m_binaryResponseBuilder.clear();
        } else {
            m_responseArrayBuffer = ArrayBuffer::create(static_cast<void*>(0), 0);
        }
    }

    return m_responseArrayBuffer.get();
}

Stream* XMLHttpRequest::responseLegacyStream()
{
    ASSERT(m_responseTypeCode == ResponseTypeLegacyStream);

    if (m_error || (m_state != LOADING && m_state != DONE))
        return 0;

    return m_responseLegacyStream.get();
}

ReadableStream* XMLHttpRequest::responseStream()
{
    ASSERT(m_responseTypeCode == ResponseTypeStream);
    if (m_error || (m_state != LOADING && m_state != DONE))
        return 0;

    return m_responseStream;
}

void XMLHttpRequest::setTimeout(unsigned long timeout, ExceptionState& exceptionState)
{
    // FIXME: Need to trigger or update the timeout Timer here, if needed. http://webkit.org/b/98156
    // XHR2 spec, 4.7.3. "This implies that the timeout attribute can be set while fetching is in progress. If that occurs it will still be measured relative to the start of fetching."
    if (executionContext()->isDocument() && !m_async) {
        exceptionState.throwDOMException(InvalidAccessError, "Timeouts cannot be set for synchronous requests made from a document.");
        return;
    }

    m_timeoutMilliseconds = timeout;

    // From http://www.w3.org/TR/XMLHttpRequest/#the-timeout-attribute:
    // Note: This implies that the timeout attribute can be set while fetching is in progress. If
    // that occurs it will still be measured relative to the start of fetching.
    //
    // The timeout may be overridden after send.
    if (m_loader)
        m_loader->overrideTimeout(timeout);
}

void XMLHttpRequest::setResponseType(const String& responseType, ExceptionState& exceptionState)
{
    if (m_state >= LOADING) {
        exceptionState.throwDOMException(InvalidStateError, "The response type cannot be set if the object's state is LOADING or DONE.");
        return;
    }

    // Newer functionality is not available to synchronous requests in window contexts, as a spec-mandated
    // attempt to discourage synchronous XHR use. responseType is one such piece of functionality.
    if (!m_async && executionContext()->isDocument()) {
        exceptionState.throwDOMException(InvalidAccessError, "The response type cannot be changed for synchronous requests made from a document.");
        return;
    }

    if (responseType == "") {
        m_responseTypeCode = ResponseTypeDefault;
    } else if (responseType == "text") {
        m_responseTypeCode = ResponseTypeText;
    } else if (responseType == "json") {
        m_responseTypeCode = ResponseTypeJSON;
    } else if (responseType == "document") {
        m_responseTypeCode = ResponseTypeDocument;
    } else if (responseType == "blob") {
        m_responseTypeCode = ResponseTypeBlob;
    } else if (responseType == "arraybuffer") {
        m_responseTypeCode = ResponseTypeArrayBuffer;
    } else if (responseType == "legacystream") {
        if (RuntimeEnabledFeatures::streamEnabled())
            m_responseTypeCode = ResponseTypeLegacyStream;
        else
            return;
    } else if (responseType == "stream") {
        if (RuntimeEnabledFeatures::streamEnabled())
            m_responseTypeCode = ResponseTypeStream;
        else
            return;
    } else {
        ASSERT_NOT_REACHED();
    }
}

String XMLHttpRequest::responseType()
{
    switch (m_responseTypeCode) {
    case ResponseTypeDefault:
        return "";
    case ResponseTypeText:
        return "text";
    case ResponseTypeJSON:
        return "json";
    case ResponseTypeDocument:
        return "document";
    case ResponseTypeBlob:
        return "blob";
    case ResponseTypeArrayBuffer:
        return "arraybuffer";
    case ResponseTypeLegacyStream:
        return "legacystream";
    case ResponseTypeStream:
        return "stream";
    }
    return "";
}

String XMLHttpRequest::responseURL()
{
    return m_response.url().string();
}

XMLHttpRequestUpload* XMLHttpRequest::upload()
{
    if (!m_upload)
        m_upload = XMLHttpRequestUpload::create(this);
    return m_upload.get();
}

void XMLHttpRequest::trackProgress(long long length)
{
    m_receivedLength += length;

    if (m_state != LOADING) {
        changeState(LOADING);
    } else {
        // Dispatch a readystatechange event because many applications use
        // it to track progress although this is not specified.
        //
        // FIXME: Stop dispatching this event for progress tracking.
        dispatchReadyStateChangeEvent();
    }
    if (m_async)
        dispatchProgressEventFromSnapshot(EventTypeNames::progress);
}

void XMLHttpRequest::changeState(State newState)
{
    if (m_state != newState) {
        m_state = newState;
        dispatchReadyStateChangeEvent();
    }
}

void XMLHttpRequest::dispatchReadyStateChangeEvent()
{
    if (!executionContext())
        return;

    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willDispatchXHRReadyStateChangeEvent(executionContext(), this);

    if (m_async || (m_state <= OPENED || m_state == DONE)) {
        TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline"), "XHRReadyStateChange", "data", InspectorXhrReadyStateChangeEvent::data(executionContext(), this));
        TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline.stack"), "CallStack", "stack", InspectorCallStackEvent::currentCallStack());
        XMLHttpRequestProgressEventThrottle::DeferredEventAction action = XMLHttpRequestProgressEventThrottle::Ignore;
        if (m_state == DONE) {
            if (m_error)
                action = XMLHttpRequestProgressEventThrottle::Clear;
            else
                action = XMLHttpRequestProgressEventThrottle::Flush;
        }
        m_progressEventThrottle.dispatchReadyStateChangeEvent(Event::create(EventTypeNames::readystatechange), action);
        TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline"), "UpdateCounters", "data", InspectorUpdateCountersEvent::data());
    }

    InspectorInstrumentation::didDispatchXHRReadyStateChangeEvent(cookie);
    if (m_state == DONE && !m_error) {
        {
            TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline"), "XHRLoad", "data", InspectorXhrLoadEvent::data(executionContext(), this));
            TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline.stack"), "CallStack", "stack", InspectorCallStackEvent::currentCallStack());
            InspectorInstrumentationCookie cookie = InspectorInstrumentation::willDispatchXHRLoadEvent(executionContext(), this);
            dispatchProgressEventFromSnapshot(EventTypeNames::load);
            InspectorInstrumentation::didDispatchXHRLoadEvent(cookie);
        }
        dispatchProgressEventFromSnapshot(EventTypeNames::loadend);
        TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline"), "UpdateCounters", "data", InspectorUpdateCountersEvent::data());
    }
}

void XMLHttpRequest::setWithCredentials(bool value, ExceptionState& exceptionState)
{
    if (m_state > OPENED || m_loader) {
        exceptionState.throwDOMException(InvalidStateError,  "The value may only be set if the object's state is UNSENT or OPENED.");
        return;
    }

    // FIXME: According to XMLHttpRequest Level 2 we should throw InvalidAccessError exception here.
    // However for time being only print warning message to warn web developers.
    if (!m_async)
        UseCounter::countDeprecation(executionContext(), UseCounter::SyncXHRWithCredentials);

    m_includeCredentials = value;
}

AtomicString XMLHttpRequest::uppercaseKnownHTTPMethod(const AtomicString& method)
{
    // Valid methods per step-5 of http://xhr.spec.whatwg.org/#the-open()-method.
    const char* const methods[] = {
        "DELETE",
        "GET",
        "HEAD",
        "OPTIONS",
        "POST",
        "PUT" };

    for (unsigned i = 0; i < WTF_ARRAY_LENGTH(methods); ++i) {
        if (equalIgnoringCase(method, methods[i])) {
            // Don't bother allocating a new string if it's already all uppercase.
            if (method == methods[i])
                return method;
            return methods[i];
        }
    }
    return method;
}

void XMLHttpRequest::open(const AtomicString& method, const KURL& url, ExceptionState& exceptionState)
{
    open(method, url, true, exceptionState);
}

void XMLHttpRequest::open(const AtomicString& method, const KURL& url, bool async, ExceptionState& exceptionState)
{
    WTF_LOG(Network, "XMLHttpRequest %p open('%s', '%s', %d)", this, method.utf8().data(), url.elidedString().utf8().data(), async);

    if (!internalAbort())
        return;

    State previousState = m_state;
    m_state = UNSENT;
    m_error = false;
    m_uploadComplete = false;

    if (!isValidHTTPToken(method)) {
        exceptionState.throwDOMException(SyntaxError, "'" + method + "' is not a valid HTTP method.");
        return;
    }

    if (FetchUtils::isForbiddenMethod(method)) {
        exceptionState.throwSecurityError("'" + method + "' HTTP method is unsupported.");
        return;
    }

    if (!ContentSecurityPolicy::shouldBypassMainWorld(executionContext()) && !executionContext()->contentSecurityPolicy()->allowConnectToSource(url)) {
        // We can safely expose the URL to JavaScript, as these checks happen synchronously before redirection. JavaScript receives no new information.
        exceptionState.throwSecurityError("Refused to connect to '" + url.elidedString() + "' because it violates the document's Content Security Policy.");
        return;
    }

    if (!async && executionContext()->isDocument()) {
        if (document()->settings() && !document()->settings()->syncXHRInDocumentsEnabled()) {
            exceptionState.throwDOMException(InvalidAccessError, "Synchronous requests are disabled for this page.");
            return;
        }

        // Newer functionality is not available to synchronous requests in window contexts, as a spec-mandated
        // attempt to discourage synchronous XHR use. responseType is one such piece of functionality.
        if (m_responseTypeCode != ResponseTypeDefault) {
            exceptionState.throwDOMException(InvalidAccessError, "Synchronous requests from a document must not set a response type.");
            return;
        }

        // Similarly, timeouts are disabled for synchronous requests as well.
        if (m_timeoutMilliseconds > 0) {
            exceptionState.throwDOMException(InvalidAccessError, "Synchronous requests must not set a timeout.");
            return;
        }
    }

    m_method = uppercaseKnownHTTPMethod(method);

    m_url = url;

    m_async = async;

    ASSERT(!m_loader);

    // Check previous state to avoid dispatching readyState event
    // when calling open several times in a row.
    if (previousState != OPENED)
        changeState(OPENED);
    else
        m_state = OPENED;
}

void XMLHttpRequest::open(const AtomicString& method, const KURL& url, bool async, const String& user, ExceptionState& exceptionState)
{
    KURL urlWithCredentials(url);
    urlWithCredentials.setUser(user);

    open(method, urlWithCredentials, async, exceptionState);
}

void XMLHttpRequest::open(const AtomicString& method, const KURL& url, bool async, const String& user, const String& password, ExceptionState& exceptionState)
{
    KURL urlWithCredentials(url);
    urlWithCredentials.setUser(user);
    urlWithCredentials.setPass(password);

    open(method, urlWithCredentials, async, exceptionState);
}

bool XMLHttpRequest::initSend(ExceptionState& exceptionState)
{
    if (!executionContext())
        return false;

    if (m_state != OPENED || m_loader) {
        exceptionState.throwDOMException(InvalidStateError, "The object's state must be OPENED.");
        return false;
    }

    m_error = false;
    return true;
}

void XMLHttpRequest::send(ExceptionState& exceptionState)
{
    send(String(), exceptionState);
}

bool XMLHttpRequest::areMethodAndURLValidForSend()
{
    return m_method != "GET" && m_method != "HEAD" && m_url.protocolIsInHTTPFamily();
}

void XMLHttpRequest::send(Document* document, ExceptionState& exceptionState)
{
    WTF_LOG(Network, "XMLHttpRequest %p send() Document %p", this, document);

    ASSERT(document);

    if (!initSend(exceptionState))
        return;

    RefPtr<FormData> httpBody;

    if (areMethodAndURLValidForSend()) {
        if (getRequestHeader("Content-Type").isEmpty()) {
            // FIXME: this should include the charset used for encoding.
            setRequestHeaderInternal("Content-Type", "application/xml");
        }

        // FIXME: According to XMLHttpRequest Level 2, this should use the Document.innerHTML algorithm
        // from the HTML5 specification to serialize the document.
        String body = createMarkup(document);

        // FIXME: This should use value of document.inputEncoding to determine the encoding to use.
        httpBody = FormData::create(UTF8Encoding().encode(body, WTF::EntitiesForUnencodables));
        if (m_upload)
            httpBody->setAlwaysStream(true);
    }

    createRequest(httpBody.release(), exceptionState);
}

void XMLHttpRequest::send(const String& body, ExceptionState& exceptionState)
{
    WTF_LOG(Network, "XMLHttpRequest %p send() String '%s'", this, body.utf8().data());

    if (!initSend(exceptionState))
        return;

    RefPtr<FormData> httpBody;

    if (!body.isNull() && areMethodAndURLValidForSend()) {
        String contentType = getRequestHeader("Content-Type");
        if (contentType.isEmpty()) {
            setRequestHeaderInternal("Content-Type", "text/plain;charset=UTF-8");
        } else {
            replaceCharsetInMediaType(contentType, "UTF-8");
            m_requestHeaders.set("Content-Type", AtomicString(contentType));
        }

        httpBody = FormData::create(UTF8Encoding().encode(body, WTF::EntitiesForUnencodables));
        if (m_upload)
            httpBody->setAlwaysStream(true);
    }

    createRequest(httpBody.release(), exceptionState);
}

void XMLHttpRequest::send(Blob* body, ExceptionState& exceptionState)
{
    WTF_LOG(Network, "XMLHttpRequest %p send() Blob '%s'", this, body->uuid().utf8().data());

    if (!initSend(exceptionState))
        return;

    RefPtr<FormData> httpBody;

    if (areMethodAndURLValidForSend()) {
        if (getRequestHeader("Content-Type").isEmpty()) {
            const String& blobType = body->type();
            if (!blobType.isEmpty() && isValidContentType(blobType)) {
                setRequestHeaderInternal("Content-Type", AtomicString(blobType));
            } else {
                // From FileAPI spec, whenever media type cannot be determined,
                // empty string must be returned.
                setRequestHeaderInternal("Content-Type", "");
            }
        }

        // FIXME: add support for uploading bundles.
        httpBody = FormData::create();
        if (body->hasBackingFile()) {
            File* file = toFile(body);
            if (!file->path().isEmpty())
                httpBody->appendFile(file->path());
            else if (!file->fileSystemURL().isEmpty())
                httpBody->appendFileSystemURL(file->fileSystemURL());
            else
                ASSERT_NOT_REACHED();
        } else {
            httpBody->appendBlob(body->uuid(), body->blobDataHandle());
        }
    }

    createRequest(httpBody.release(), exceptionState);
}

void XMLHttpRequest::send(DOMFormData* body, ExceptionState& exceptionState)
{
    WTF_LOG(Network, "XMLHttpRequest %p send() DOMFormData %p", this, body);

    if (!initSend(exceptionState))
        return;

    RefPtr<FormData> httpBody;

    if (areMethodAndURLValidForSend()) {
        httpBody = body->createMultiPartFormData();

        if (getRequestHeader("Content-Type").isEmpty()) {
            AtomicString contentType = AtomicString("multipart/form-data; boundary=", AtomicString::ConstructFromLiteral) + httpBody->boundary().data();
            setRequestHeaderInternal("Content-Type", contentType);
        }
    }

    createRequest(httpBody.release(), exceptionState);
}

void XMLHttpRequest::send(ArrayBuffer* body, ExceptionState& exceptionState)
{
    WTF_LOG(Network, "XMLHttpRequest %p send() ArrayBuffer %p", this, body);

    sendBytesData(body->data(), body->byteLength(), exceptionState);
}

void XMLHttpRequest::send(ArrayBufferView* body, ExceptionState& exceptionState)
{
    WTF_LOG(Network, "XMLHttpRequest %p send() ArrayBufferView %p", this, body);

    sendBytesData(body->baseAddress(), body->byteLength(), exceptionState);
}

void XMLHttpRequest::sendBytesData(const void* data, size_t length, ExceptionState& exceptionState)
{
    if (!initSend(exceptionState))
        return;

    RefPtr<FormData> httpBody;

    if (areMethodAndURLValidForSend()) {
        httpBody = FormData::create(data, length);
        if (m_upload)
            httpBody->setAlwaysStream(true);
    }

    createRequest(httpBody.release(), exceptionState);
}

void XMLHttpRequest::sendForInspectorXHRReplay(PassRefPtr<FormData> formData, ExceptionState& exceptionState)
{
    createRequest(formData ? formData->deepCopy() : nullptr, exceptionState);
    m_exceptionCode = exceptionState.code();
}

void XMLHttpRequest::createRequest(PassRefPtr<FormData> httpBody, ExceptionState& exceptionState)
{
    // Only GET request is supported for blob URL.
    if (m_url.protocolIs("blob") && m_method != "GET") {
        exceptionState.throwDOMException(NetworkError, "'GET' is the only method allowed for 'blob:' URLs.");
        return;
    }

    // The presence of upload event listeners forces us to use preflighting because POSTing to an URL that does not
    // permit cross origin requests should look exactly like POSTing to an URL that does not respond at all.
    // Also, only async requests support upload progress events.
    bool uploadEvents = false;
    if (m_async) {
        dispatchProgressEvent(EventTypeNames::loadstart, 0, 0);
        if (httpBody && m_upload) {
            uploadEvents = m_upload->hasEventListeners();
            m_upload->dispatchEvent(XMLHttpRequestProgressEvent::create(EventTypeNames::loadstart));
        }
    }

    m_sameOriginRequest = securityOrigin()->canRequest(m_url);

    // We also remember whether upload events should be allowed for this request in case the upload listeners are
    // added after the request is started.
    m_uploadEventsAllowed = m_sameOriginRequest || uploadEvents || !FetchUtils::isSimpleRequest(m_method, m_requestHeaders);

    ASSERT(executionContext());
    ExecutionContext& executionContext = *this->executionContext();

    ResourceRequest request(m_url);
    request.setHTTPMethod(m_method);
    request.setRequestContext(blink::WebURLRequest::RequestContextXMLHttpRequest);

    InspectorInstrumentation::willLoadXHR(&executionContext, this, this, m_method, m_url, m_async, httpBody ? httpBody->deepCopy() : nullptr, m_requestHeaders, m_includeCredentials);

    if (httpBody) {
        ASSERT(m_method != "GET");
        ASSERT(m_method != "HEAD");
        request.setHTTPBody(httpBody);
    }

    if (m_requestHeaders.size() > 0)
        request.addHTTPHeaderFields(m_requestHeaders);

    ThreadableLoaderOptions options;
    options.preflightPolicy = uploadEvents ? ForcePreflight : ConsiderPreflight;
    options.crossOriginRequestPolicy = UseAccessControl;
    options.initiator = FetchInitiatorTypeNames::xmlhttprequest;
    options.contentSecurityPolicyEnforcement = ContentSecurityPolicy::shouldBypassMainWorld(&executionContext) ? DoNotEnforceContentSecurityPolicy : EnforceConnectSrcDirective;
    options.timeoutMilliseconds = m_timeoutMilliseconds;

    ResourceLoaderOptions resourceLoaderOptions;
    resourceLoaderOptions.allowCredentials = (m_sameOriginRequest || m_includeCredentials) ? AllowStoredCredentials : DoNotAllowStoredCredentials;
    resourceLoaderOptions.credentialsRequested = m_includeCredentials ? ClientRequestedCredentials : ClientDidNotRequestCredentials;
    resourceLoaderOptions.securityOrigin = securityOrigin();
    resourceLoaderOptions.mixedContentBlockingTreatment = RuntimeEnabledFeatures::laxMixedContentCheckingEnabled() ? TreatAsPassiveContent : TreatAsActiveContent;

    // When responseType is set to "blob", we redirect the downloaded data to a
    // file-handle directly.
    m_downloadingToFile = responseTypeCode() == ResponseTypeBlob;
    if (m_downloadingToFile) {
        request.setDownloadToFile(true);
        resourceLoaderOptions.dataBufferingPolicy = DoNotBufferData;
    }

    m_exceptionCode = 0;
    m_error = false;

    if (m_async) {
        if (m_upload)
            request.setReportUploadProgress(true);

        // ThreadableLoader::create can return null here, for example if we're no longer attached to a page.
        // This is true while running onunload handlers.
        // FIXME: Maybe we need to be able to send XMLHttpRequests from onunload, <http://bugs.webkit.org/show_bug.cgi?id=10904>.
        // FIXME: Maybe create() can return null for other reasons too?
        ASSERT(!m_loader);
        m_loader = ThreadableLoader::create(executionContext, this, request, options, resourceLoaderOptions);
    } else {
        // Use count for XHR synchronous requests.
        UseCounter::count(&executionContext, UseCounter::XMLHttpRequestSynchronous);
        ThreadableLoader::loadResourceSynchronously(executionContext, request, *this, options, resourceLoaderOptions);
    }

    if (!m_exceptionCode && m_error)
        m_exceptionCode = NetworkError;
    if (m_exceptionCode)
        exceptionState.throwDOMException(m_exceptionCode, "Failed to load '" + m_url.elidedString() + "'.");
}

void XMLHttpRequest::abort()
{
    WTF_LOG(Network, "XMLHttpRequest %p abort()", this);

    // internalAbort() clears |m_loader|. Compute |sendFlag| now.
    //
    // |sendFlag| corresponds to "the send() flag" defined in the XHR spec.
    //
    // |sendFlag| is only set when we have an active, asynchronous loader.
    // Don't use it as "the send() flag" when the XHR is in sync mode.
    bool sendFlag = m_loader;

    // internalAbort() clears the response. Save the data needed for
    // dispatching ProgressEvents.
    long long expectedLength = m_response.expectedContentLength();
    long long receivedLength = m_receivedLength;

    if (!internalAbort())
        return;

    // The script never gets any chance to call abort() on a sync XHR between
    // send() call and transition to the DONE state. It's because a sync XHR
    // doesn't dispatch any event between them. So, if |m_async| is false, we
    // can skip the "request error steps" (defined in the XHR spec) without any
    // state check.
    //
    // FIXME: It's possible open() is invoked in internalAbort() and |m_async|
    // becomes true by that. We should implement more reliable treatment for
    // nested method invocations at some point.
    if (m_async) {
        if ((m_state == OPENED && sendFlag) || m_state == HEADERS_RECEIVED || m_state == LOADING) {
            ASSERT(!m_loader);
            handleRequestError(0, EventTypeNames::abort, receivedLength, expectedLength);
        }
    }
    m_state = UNSENT;
}

void XMLHttpRequest::clearVariablesForLoading()
{
    m_decoder.clear();

    if (m_responseDocumentParser) {
        m_responseDocumentParser->removeClient(this);
#if !ENABLE(OILPAN)
        m_responseDocumentParser->detach();
#endif
        m_responseDocumentParser = nullptr;
    }

    m_finalResponseCharset = String();
}

bool XMLHttpRequest::internalAbort()
{
    m_error = true;

    if (m_responseDocumentParser && !m_responseDocumentParser->isStopped())
        m_responseDocumentParser->stopParsing();

    clearVariablesForLoading();

    InspectorInstrumentation::didFailXHRLoading(executionContext(), this, this);

    if (m_responseLegacyStream && m_state != DONE)
        m_responseLegacyStream->abort();

    if (m_responseStream) {
        // When the stream is already closed (including canceled from the
        // user), |error| does nothing.
        // FIXME: Create a more specific error.
        m_responseStream->error(DOMException::create(!m_async && m_exceptionCode ? m_exceptionCode : AbortError, "XMLHttpRequest::abort"));
    }

    clearResponse();
    clearRequest();

    if (!m_loader)
        return true;

    // Cancelling the ThreadableLoader m_loader may result in calling
    // window.onload synchronously. If such an onload handler contains open()
    // call on the same XMLHttpRequest object, reentry happens.
    //
    // If, window.onload contains open() and send(), m_loader will be set to
    // non 0 value. So, we cannot continue the outer open(). In such case,
    // just abort the outer open() by returning false.
    RefPtrWillBeRawPtr<XMLHttpRequest> protect(this);
    RefPtr<ThreadableLoader> loader = m_loader.release();
    loader->cancel();

    // If abort() called internalAbort() and a nested open() ended up
    // clearing the error flag, but didn't send(), make sure the error
    // flag is still set.
    bool newLoadStarted = hasPendingActivity();
    if (!newLoadStarted)
        m_error = true;

    return !newLoadStarted;
}

void XMLHttpRequest::clearResponse()
{
    // FIXME: when we add the support for multi-part XHR, we will have to
    // be careful with this initialization.
    m_receivedLength = 0;

    m_response = ResourceResponse();

    m_responseText.clear();

    m_parsedResponse = false;
    m_responseDocument = nullptr;

    m_responseBlob = nullptr;

    m_downloadingToFile = false;
    m_lengthDownloadedToFile = 0;

    m_responseLegacyStream = nullptr;
    m_responseStream = nullptr;

    // These variables may referred by the response accessors. So, we can clear
    // this only when we clear the response holder variables above.
    m_binaryResponseBuilder.clear();
    m_responseArrayBuffer.clear();
}

void XMLHttpRequest::clearRequest()
{
    m_requestHeaders.clear();
}

void XMLHttpRequest::dispatchProgressEvent(const AtomicString& type, long long receivedLength, long long expectedLength)
{
    bool lengthComputable = expectedLength > 0 && receivedLength <= expectedLength;
    unsigned long long loaded = receivedLength >= 0 ? static_cast<unsigned long long>(receivedLength) : 0;
    unsigned long long total = lengthComputable ? static_cast<unsigned long long>(expectedLength) : 0;

    m_progressEventThrottle.dispatchProgressEvent(type, lengthComputable, loaded, total);

    if (type == EventTypeNames::loadend)
        InspectorInstrumentation::didDispatchXHRLoadendEvent(executionContext(), this);
}

void XMLHttpRequest::dispatchProgressEventFromSnapshot(const AtomicString& type)
{
    dispatchProgressEvent(type, m_receivedLength, m_response.expectedContentLength());
}

void XMLHttpRequest::handleNetworkError()
{
    WTF_LOG(Network, "XMLHttpRequest %p handleNetworkError()", this);

    // Response is cleared next, save needed progress event data.
    long long expectedLength = m_response.expectedContentLength();
    long long receivedLength = m_receivedLength;

    if (!internalAbort())
        return;

    handleRequestError(NetworkError, EventTypeNames::error, receivedLength, expectedLength);
}

void XMLHttpRequest::handleDidCancel()
{
    WTF_LOG(Network, "XMLHttpRequest %p handleDidCancel()", this);

    // Response is cleared next, save needed progress event data.
    long long expectedLength = m_response.expectedContentLength();
    long long receivedLength = m_receivedLength;

    if (!internalAbort())
        return;

    handleRequestError(AbortError, EventTypeNames::abort, receivedLength, expectedLength);
}

void XMLHttpRequest::handleRequestError(ExceptionCode exceptionCode, const AtomicString& type, long long receivedLength, long long expectedLength)
{
    WTF_LOG(Network, "XMLHttpRequest %p handleRequestError()", this);

    // The request error steps for event 'type' and exception 'exceptionCode'.

    if (!m_async && exceptionCode) {
        m_state = DONE;
        m_exceptionCode = exceptionCode;
        return;
    }
    // With m_error set, the state change steps are minimal: any pending
    // progress event is flushed + a readystatechange is dispatched.
    // No new progress events dispatched; as required, that happens at
    // the end here.
    ASSERT(m_error);
    changeState(DONE);

    if (!m_uploadComplete) {
        m_uploadComplete = true;
        if (m_upload && m_uploadEventsAllowed)
            m_upload->handleRequestError(type);
    }

    // Note: The below event dispatch may be called while |hasPendingActivity() == false|,
    // when |handleRequestError| is called after |internalAbort()|.
    // This is safe, however, as |this| will be kept alive from a strong ref |Event::m_target|.
    dispatchProgressEvent(EventTypeNames::progress, receivedLength, expectedLength);
    dispatchProgressEvent(type, receivedLength, expectedLength);
    dispatchProgressEvent(EventTypeNames::loadend, receivedLength, expectedLength);
}

void XMLHttpRequest::overrideMimeType(const AtomicString& mimeType, ExceptionState& exceptionState)
{
    if (m_state == LOADING || m_state == DONE) {
        exceptionState.throwDOMException(InvalidStateError, "MimeType cannot be overridden when the state is LOADING or DONE.");
        return;
    }

    m_mimeTypeOverride = mimeType;
}

void XMLHttpRequest::setRequestHeader(const AtomicString& name, const AtomicString& value, ExceptionState& exceptionState)
{
    if (m_state != OPENED || m_loader) {
        exceptionState.throwDOMException(InvalidStateError, "The object's state must be OPENED.");
        return;
    }

    if (!isValidHTTPToken(name)) {
        exceptionState.throwDOMException(SyntaxError, "'" + name + "' is not a valid HTTP header field name.");
        return;
    }

    if (!isValidHTTPHeaderValue(value)) {
        exceptionState.throwDOMException(SyntaxError, "'" + value + "' is not a valid HTTP header field value.");
        return;
    }

    // No script (privileged or not) can set unsafe headers.
    if (FetchUtils::isForbiddenHeaderName(name)) {
        logConsoleError(executionContext(), "Refused to set unsafe header \"" + name + "\"");
        return;
    }

    setRequestHeaderInternal(name, value);
}

void XMLHttpRequest::setRequestHeaderInternal(const AtomicString& name, const AtomicString& value)
{
    HTTPHeaderMap::AddResult result = m_requestHeaders.add(name, value);
    if (!result.isNewEntry)
        result.storedValue->value = result.storedValue->value + ", " + value;
}

const AtomicString& XMLHttpRequest::getRequestHeader(const AtomicString& name) const
{
    return m_requestHeaders.get(name);
}

String XMLHttpRequest::getAllResponseHeaders() const
{
    if (m_state < HEADERS_RECEIVED || m_error)
        return "";

    StringBuilder stringBuilder;

    HTTPHeaderSet accessControlExposeHeaderSet;
    parseAccessControlExposeHeadersAllowList(m_response.httpHeaderField("Access-Control-Expose-Headers"), accessControlExposeHeaderSet);
    HTTPHeaderMap::const_iterator end = m_response.httpHeaderFields().end();
    for (HTTPHeaderMap::const_iterator it = m_response.httpHeaderFields().begin(); it!= end; ++it) {
        // Hide Set-Cookie header fields from the XMLHttpRequest client for these reasons:
        //     1) If the client did have access to the fields, then it could read HTTP-only
        //        cookies; those cookies are supposed to be hidden from scripts.
        //     2) There's no known harm in hiding Set-Cookie header fields entirely; we don't
        //        know any widely used technique that requires access to them.
        //     3) Firefox has implemented this policy.
        if (isSetCookieHeader(it->key) && !securityOrigin()->canLoadLocalResources())
            continue;

        if (!m_sameOriginRequest && !isOnAccessControlResponseHeaderWhitelist(it->key) && !accessControlExposeHeaderSet.contains(it->key))
            continue;

        stringBuilder.append(it->key);
        stringBuilder.append(':');
        stringBuilder.append(' ');
        stringBuilder.append(it->value);
        stringBuilder.append('\r');
        stringBuilder.append('\n');
    }

    return stringBuilder.toString();
}

const AtomicString& XMLHttpRequest::getResponseHeader(const AtomicString& name) const
{
    if (m_state < HEADERS_RECEIVED || m_error)
        return nullAtom;

    // See comment in getAllResponseHeaders above.
    if (isSetCookieHeader(name) && !securityOrigin()->canLoadLocalResources()) {
        logConsoleError(executionContext(), "Refused to get unsafe header \"" + name + "\"");
        return nullAtom;
    }

    HTTPHeaderSet accessControlExposeHeaderSet;
    parseAccessControlExposeHeadersAllowList(m_response.httpHeaderField("Access-Control-Expose-Headers"), accessControlExposeHeaderSet);

    if (!m_sameOriginRequest && !isOnAccessControlResponseHeaderWhitelist(name) && !accessControlExposeHeaderSet.contains(name)) {
        logConsoleError(executionContext(), "Refused to get unsafe header \"" + name + "\"");
        return nullAtom;
    }
    return m_response.httpHeaderField(name);
}

AtomicString XMLHttpRequest::finalResponseMIMEType() const
{
    AtomicString overriddenType = extractMIMETypeFromMediaType(m_mimeTypeOverride);
    if (!overriddenType.isEmpty())
        return overriddenType;

    if (m_response.isHTTP())
        return extractMIMETypeFromMediaType(m_response.httpHeaderField("Content-Type"));

    return m_response.mimeType();
}

AtomicString XMLHttpRequest::finalResponseMIMETypeWithFallback() const
{
    AtomicString finalType = finalResponseMIMEType();
    if (!finalType.isEmpty())
        return finalType;

    // FIXME: This fallback is not specified in the final MIME type algorithm
    // of the XHR spec. Move this to more appropriate place.
    return AtomicString("text/xml", AtomicString::ConstructFromLiteral);
}

bool XMLHttpRequest::responseIsXML() const
{
    return DOMImplementation::isXMLMIMEType(finalResponseMIMETypeWithFallback());
}

bool XMLHttpRequest::responseIsHTML() const
{
    return equalIgnoringCase(finalResponseMIMEType(), "text/html");
}

int XMLHttpRequest::status() const
{
    if (m_state == UNSENT || m_state == OPENED || m_error)
        return 0;

    if (m_response.httpStatusCode())
        return m_response.httpStatusCode();

    return 0;
}

String XMLHttpRequest::statusText() const
{
    if (m_state == UNSENT || m_state == OPENED || m_error)
        return String();

    if (!m_response.httpStatusText().isNull())
        return m_response.httpStatusText();

    return String();
}

void XMLHttpRequest::didFail(const ResourceError& error)
{
    WTF_LOG(Network, "XMLHttpRequest %p didFail()", this);

    // If we are already in an error state, for instance we called abort(), bail out early.
    if (m_error)
        return;

    if (error.isCancellation()) {
        handleDidCancel();
        return;
    }

    if (error.isTimeout()) {
        handleDidTimeout();
        return;
    }

    // Network failures are already reported to Web Inspector by ResourceLoader.
    if (error.domain() == errorDomainBlinkInternal)
        logConsoleError(executionContext(), "XMLHttpRequest cannot load " + error.failingURL() + ". " + error.localizedDescription());

    handleNetworkError();
}

void XMLHttpRequest::didFailRedirectCheck()
{
    WTF_LOG(Network, "XMLHttpRequest %p didFailRedirectCheck()", this);

    handleNetworkError();
}

void XMLHttpRequest::didFinishLoading(unsigned long identifier, double)
{
    WTF_LOG(Network, "XMLHttpRequest %p didFinishLoading(%lu)", this, identifier);

    if (m_error)
        return;

    if (m_state < HEADERS_RECEIVED)
        changeState(HEADERS_RECEIVED);

    m_loaderIdentifier = identifier;

    if (m_responseDocumentParser) {
        // |DocumentParser::finish()| tells the parser that we have reached end of the data.
        // When using |HTMLDocumentParser|, which works asynchronously, we do not have the
        // complete document just after the |DocumentParser::finish()| call.
        // Wait for the parser to call us back in |notifyParserStopped| to progress state.
        m_responseDocumentParser->finish();
        ASSERT(m_responseDocument);
        return;
    }

    if (m_decoder)
        m_responseText = m_responseText.concatenateWith(m_decoder->flush());

    if (m_responseLegacyStream)
        m_responseLegacyStream->finalize();

    if (m_responseStream)
        m_responseStream->close();

    clearVariablesForLoading();
    endLoading();
}

void XMLHttpRequest::notifyParserStopped()
{
    // This should only be called when response document is parsed asynchronously.
    ASSERT(m_responseDocumentParser);
    ASSERT(!m_responseDocumentParser->isParsing());
    ASSERT(!m_responseLegacyStream);
    ASSERT(!m_responseStream);

    // Do nothing if we are called from |internalAbort()|.
    if (m_error)
        return;

    clearVariablesForLoading();

    m_responseDocument->implicitClose();

    if (!m_responseDocument->wellFormed())
        m_responseDocument = nullptr;

    m_parsedResponse = true;

    endLoading();
}

void XMLHttpRequest::endLoading()
{
    InspectorInstrumentation::didFinishXHRLoading(executionContext(), this, this, m_loaderIdentifier, m_responseText, m_method, m_url, m_lastSendURL, m_lastSendLineNumber);

    if (m_loader)
        m_loader = nullptr;
    m_loaderIdentifier = 0;

    changeState(DONE);
}

void XMLHttpRequest::didSendData(unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    WTF_LOG(Network, "XMLHttpRequest %p didSendData(%llu, %llu)", this, bytesSent, totalBytesToBeSent);

    if (!m_upload)
        return;

    if (m_uploadEventsAllowed)
        m_upload->dispatchProgressEvent(bytesSent, totalBytesToBeSent);

    if (bytesSent == totalBytesToBeSent && !m_uploadComplete) {
        m_uploadComplete = true;
        if (m_uploadEventsAllowed)
            m_upload->dispatchEventAndLoadEnd(EventTypeNames::load, true, bytesSent, totalBytesToBeSent);
    }
}

void XMLHttpRequest::didReceiveResponse(unsigned long identifier, const ResourceResponse& response)
{
    WTF_LOG(Network, "XMLHttpRequest %p didReceiveResponse(%lu)", this, identifier);

    m_response = response;
    if (!m_mimeTypeOverride.isEmpty()) {
        m_response.setHTTPHeaderField("Content-Type", m_mimeTypeOverride);
        m_finalResponseCharset = extractCharsetFromMediaType(m_mimeTypeOverride);
    }

    if (m_finalResponseCharset.isEmpty())
        m_finalResponseCharset = response.textEncodingName();
}

void XMLHttpRequest::parseDocumentChunk(const char* data, unsigned len)
{
    if (!m_responseDocumentParser) {
        ASSERT(!m_responseDocument);
        initResponseDocument();
        if (!m_responseDocument)
            return;

        m_responseDocumentParser = m_responseDocument->implicitOpen();
        m_responseDocumentParser->addClient(this);
    }
    ASSERT(m_responseDocumentParser);

    if (m_responseDocumentParser->needsDecoder())
        m_responseDocumentParser->setDecoder(createDecoder());

    m_responseDocumentParser->appendBytes(data, len);
}

PassOwnPtr<TextResourceDecoder> XMLHttpRequest::createDecoder() const
{
    if (m_responseTypeCode == ResponseTypeJSON)
        return TextResourceDecoder::create("application/json", "UTF-8");

    if (!m_finalResponseCharset.isEmpty())
        return TextResourceDecoder::create("text/plain", m_finalResponseCharset);

    // allow TextResourceDecoder to look inside the m_response if it's XML or HTML
    if (responseIsXML()) {
        OwnPtr<TextResourceDecoder> decoder = TextResourceDecoder::create("application/xml");
        // Don't stop on encoding errors, unlike it is done for other kinds
        // of XML resources. This matches the behavior of previous WebKit
        // versions, Firefox and Opera.
        decoder->useLenientXMLDecoding();

        return decoder.release();
    }

    if (responseIsHTML())
        return TextResourceDecoder::create("text/html", "UTF-8");

    return TextResourceDecoder::create("text/plain", "UTF-8");
}

void XMLHttpRequest::didReceiveData(const char* data, unsigned len)
{
    ASSERT(!m_downloadingToFile);

    if (m_error)
        return;

    if (m_state < HEADERS_RECEIVED)
        changeState(HEADERS_RECEIVED);

    // We need to check for |m_error| again, because |changeState| may trigger
    // readystatechange, and user javascript can cause |abort()|.
    if (m_error)
        return;

    if (!len)
        return;

    if (m_responseTypeCode == ResponseTypeDocument && responseIsHTML()) {
        parseDocumentChunk(data, len);
    } else if (m_responseTypeCode == ResponseTypeDefault || m_responseTypeCode == ResponseTypeText || m_responseTypeCode == ResponseTypeJSON || m_responseTypeCode == ResponseTypeDocument) {
        if (!m_decoder)
            m_decoder = createDecoder();

        m_responseText = m_responseText.concatenateWith(m_decoder->decode(data, len));
    } else if (m_responseTypeCode == ResponseTypeArrayBuffer || m_responseTypeCode == ResponseTypeBlob) {
        // Buffer binary data.
        if (!m_binaryResponseBuilder)
            m_binaryResponseBuilder = SharedBuffer::create();
        m_binaryResponseBuilder->append(data, len);
    } else if (m_responseTypeCode == ResponseTypeLegacyStream) {
        if (!m_responseLegacyStream)
            m_responseLegacyStream = Stream::create(executionContext(), responseType());
        m_responseLegacyStream->addData(data, len);
    } else if (m_responseTypeCode == ResponseTypeStream) {
        if (!m_responseStream) {
            m_responseStream = new ReadableStreamImpl<ReadableStreamChunkTypeTraits<ArrayBuffer> >(executionContext(), new ReadableStreamSource(this));
            m_responseStream->didSourceStart();
        }
        m_responseStream->enqueue(ArrayBuffer::create(data, len));
    }

    trackProgress(len);
}

void XMLHttpRequest::didDownloadData(int dataLength)
{
    if (m_error)
        return;

    ASSERT(m_downloadingToFile);

    if (m_state < HEADERS_RECEIVED)
        changeState(HEADERS_RECEIVED);

    if (!dataLength)
        return;

    // readystatechange event handler may do something to put this XHR in error
    // state. We need to check m_error again here.
    if (m_error)
        return;

    m_lengthDownloadedToFile += dataLength;

    trackProgress(dataLength);
}

void XMLHttpRequest::handleDidTimeout()
{
    WTF_LOG(Network, "XMLHttpRequest %p handleDidTimeout()", this);

    // Response is cleared next, save needed progress event data.
    long long expectedLength = m_response.expectedContentLength();
    long long receivedLength = m_receivedLength;

    if (!internalAbort())
        return;

    handleRequestError(TimeoutError, EventTypeNames::timeout, receivedLength, expectedLength);
}

void XMLHttpRequest::suspend()
{
    m_progressEventThrottle.suspend();
}

void XMLHttpRequest::resume()
{
    m_progressEventThrottle.resume();
}

void XMLHttpRequest::stop()
{
    internalAbort();
}

bool XMLHttpRequest::hasPendingActivity() const
{
    // Neither this object nor the JavaScript wrapper should be deleted while
    // a request is in progress because we need to keep the listeners alive,
    // and they are referenced by the JavaScript wrapper.
    // |m_loader| is non-null while request is active and ThreadableLoaderClient
    // callbacks may be called, and |m_responseDocumentParser| is non-null while
    // DocumentParserClient callbacks may be called.
    return m_loader || m_responseDocumentParser;
}

void XMLHttpRequest::contextDestroyed()
{
    ASSERT(!m_loader);
    ActiveDOMObject::contextDestroyed();
}

const AtomicString& XMLHttpRequest::interfaceName() const
{
    return EventTargetNames::XMLHttpRequest;
}

ExecutionContext* XMLHttpRequest::executionContext() const
{
    return ActiveDOMObject::executionContext();
}

void XMLHttpRequest::trace(Visitor* visitor)
{
    visitor->trace(m_responseBlob);
    visitor->trace(m_responseLegacyStream);
    visitor->trace(m_responseStream);
    visitor->trace(m_streamSource);
    visitor->trace(m_responseDocument);
    visitor->trace(m_responseDocumentParser);
    visitor->trace(m_progressEventThrottle);
    visitor->trace(m_upload);
    XMLHttpRequestEventTarget::trace(visitor);
}

} // namespace blink
