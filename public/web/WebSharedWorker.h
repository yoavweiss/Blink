/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebSharedWorker_h
#define WebSharedWorker_h

#include "../platform/WebCommon.h"
#include "WebContentSecurityPolicy.h"

namespace blink {

class WebString;
class WebMessagePortChannel;
class WebSharedWorkerClient;
class WebURL;

// This is the interface to a SharedWorker thread.
class WebSharedWorker {
public:
    // Invoked from the worker thread to instantiate a WebSharedWorker that interacts with the WebKit worker components.
    BLINK_EXPORT static WebSharedWorker* create(WebSharedWorkerClient*);

    virtual void startWorkerContext(
        const WebURL& scriptURL,
        const WebString& name,
        const WebString& contentSecurityPolicy,
        WebContentSecurityPolicyType) = 0;

    // Sends a connect event to the SharedWorker context.
    virtual void connect(WebMessagePortChannel*) = 0;

    // Invoked to shutdown the worker when there are no more associated documents.
    virtual void terminateWorkerContext() = 0;

    // Notification when the WebCommonWorkerClient is destroyed.
    virtual void clientDestroyed() = 0;

    virtual void pauseWorkerContextOnStart() = 0;
    virtual void resumeWorkerContext() = 0;
    virtual void attachDevTools(const WebString& hostId) = 0;
    virtual void reattachDevTools(const WebString& hostId, const WebString& savedState) = 0;
    virtual void detachDevTools() = 0;
    virtual void dispatchDevToolsMessage(const WebString&) = 0;
};

} // namespace blink

#endif
