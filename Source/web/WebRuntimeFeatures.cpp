/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "config.h"
#include "public/web/WebRuntimeFeatures.h"

#include "platform/RuntimeEnabledFeatures.h"
#include "web/WebMediaPlayerClientImpl.h"

namespace blink {

void WebRuntimeFeatures::enableExperimentalFeatures(bool enable)
{
    RuntimeEnabledFeatures::setExperimentalFeaturesEnabled(enable);
}

void WebRuntimeFeatures::enableBleedingEdgeFastPaths(bool enable)
{
    ASSERT(enable);
    RuntimeEnabledFeatures::setBleedingEdgeFastPathsEnabled(enable);
    RuntimeEnabledFeatures::setSubpixelFontScalingEnabled(enable || RuntimeEnabledFeatures::subpixelFontScalingEnabled());
    RuntimeEnabledFeatures::setWebAnimationsAPIEnabled(enable);
}

void WebRuntimeFeatures::enableTestOnlyFeatures(bool enable)
{
    RuntimeEnabledFeatures::setTestFeaturesEnabled(enable);
}

void WebRuntimeFeatures::enableApplicationCache(bool enable)
{
    RuntimeEnabledFeatures::setApplicationCacheEnabled(enable);
}


void WebRuntimeFeatures::enableCompositedSelectionUpdate(bool enable)
{
    RuntimeEnabledFeatures::setCompositedSelectionUpdateEnabled(enable);
}

bool WebRuntimeFeatures::isCompositedSelectionUpdateEnabled()
{
    return RuntimeEnabledFeatures::compositedSelectionUpdateEnabled();
}

void WebRuntimeFeatures::enableDatabase(bool enable)
{
    RuntimeEnabledFeatures::setDatabaseEnabled(enable);
}

void WebRuntimeFeatures::enableDecodeToYUV(bool enable)
{
    RuntimeEnabledFeatures::setDecodeToYUVEnabled(enable);
}

void WebRuntimeFeatures::enableDisplayList2dCanvas(bool enable)
{
    RuntimeEnabledFeatures::setDisplayList2dCanvasEnabled(enable);
}

void WebRuntimeFeatures::enableEncryptedMedia(bool enable)
{
    RuntimeEnabledFeatures::setEncryptedMediaEnabled(enable);
    // FIXME: Hack to allow MediaKeyError to be enabled for either version.
    RuntimeEnabledFeatures::setEncryptedMediaAnyVersionEnabled(
        RuntimeEnabledFeatures::encryptedMediaEnabled()
        || RuntimeEnabledFeatures::prefixedEncryptedMediaEnabled());
}

bool WebRuntimeFeatures::isEncryptedMediaEnabled()
{
    return RuntimeEnabledFeatures::encryptedMediaEnabled();
}

void WebRuntimeFeatures::enablePrefixedEncryptedMedia(bool enable)
{
    RuntimeEnabledFeatures::setPrefixedEncryptedMediaEnabled(enable);
    // FIXME: Hack to allow MediaKeyError to be enabled for either version.
    RuntimeEnabledFeatures::setEncryptedMediaAnyVersionEnabled(
        RuntimeEnabledFeatures::encryptedMediaEnabled()
        || RuntimeEnabledFeatures::prefixedEncryptedMediaEnabled());
}

bool WebRuntimeFeatures::isPrefixedEncryptedMediaEnabled()
{
    return RuntimeEnabledFeatures::prefixedEncryptedMediaEnabled();
}

void WebRuntimeFeatures::enableExperimentalCanvasFeatures(bool enable)
{
    RuntimeEnabledFeatures::setExperimentalCanvasFeaturesEnabled(enable);
}

void WebRuntimeFeatures::enableFastMobileScrolling(bool enable)
{
    RuntimeEnabledFeatures::setFastMobileScrollingEnabled(enable);
}

void WebRuntimeFeatures::enableFileSystem(bool enable)
{
    RuntimeEnabledFeatures::setFileSystemEnabled(enable);
}

void WebRuntimeFeatures::enableGamepad(bool enable)
{
    RuntimeEnabledFeatures::setGamepadEnabled(enable);
}

void WebRuntimeFeatures::enableLocalStorage(bool enable)
{
    RuntimeEnabledFeatures::setLocalStorageEnabled(enable);
}

void WebRuntimeFeatures::enableMediaPlayer(bool enable)
{
    RuntimeEnabledFeatures::setMediaEnabled(enable);
}

void WebRuntimeFeatures::enableSubpixelFontScaling(bool enable)
{
    RuntimeEnabledFeatures::setSubpixelFontScalingEnabled(enable);
}

void WebRuntimeFeatures::enableMediaCapture(bool enable)
{
    RuntimeEnabledFeatures::setMediaCaptureEnabled(enable);
}

void WebRuntimeFeatures::enableMediaSource(bool enable)
{
    RuntimeEnabledFeatures::setMediaSourceEnabled(enable);
}

void WebRuntimeFeatures::enableMediaStream(bool enable)
{
    RuntimeEnabledFeatures::setMediaStreamEnabled(enable);
}

void WebRuntimeFeatures::enableNotifications(bool enable)
{
    RuntimeEnabledFeatures::setNotificationsEnabled(enable);
}

void WebRuntimeFeatures::enableNavigatorContentUtils(bool enable)
{
    RuntimeEnabledFeatures::setNavigatorContentUtilsEnabled(enable);
}

void WebRuntimeFeatures::enableNavigationTransitions(bool enable)
{
    RuntimeEnabledFeatures::setNavigationTransitionsEnabled(enable);
}

void WebRuntimeFeatures::enableNetworkInformation(bool enable)
{
    RuntimeEnabledFeatures::setNetworkInformationEnabled(enable);
}

void WebRuntimeFeatures::enableOrientationEvent(bool enable)
{
    RuntimeEnabledFeatures::setOrientationEventEnabled(enable);
}

void WebRuntimeFeatures::enablePagePopup(bool enable)
{
    RuntimeEnabledFeatures::setPagePopupEnabled(enable);
}

void WebRuntimeFeatures::enablePeerConnection(bool enable)
{
    RuntimeEnabledFeatures::setPeerConnectionEnabled(enable);
}

void WebRuntimeFeatures::enableRequestAutocomplete(bool enable)
{
    RuntimeEnabledFeatures::setRequestAutocompleteEnabled(enable);
}

void WebRuntimeFeatures::enableScreenOrientation(bool enable)
{
    RuntimeEnabledFeatures::setScreenOrientationEnabled(enable);
}

void WebRuntimeFeatures::enableScriptedSpeech(bool enable)
{
    RuntimeEnabledFeatures::setScriptedSpeechEnabled(enable);
}

void WebRuntimeFeatures::enableServiceWorker(bool enable)
{
    RuntimeEnabledFeatures::setServiceWorkerEnabled(enable);
}

void WebRuntimeFeatures::enableSessionStorage(bool enable)
{
    RuntimeEnabledFeatures::setSessionStorageEnabled(enable);
}

void WebRuntimeFeatures::enableTouch(bool enable)
{
    RuntimeEnabledFeatures::setTouchEnabled(enable);
}

void WebRuntimeFeatures::enableTouchIconLoading(bool enable)
{
    RuntimeEnabledFeatures::setTouchIconLoadingEnabled(enable);
}

void WebRuntimeFeatures::enableWebAudio(bool enable)
{
    RuntimeEnabledFeatures::setWebAudioEnabled(enable);
}

void WebRuntimeFeatures::enableWebGLDraftExtensions(bool enable)
{
    RuntimeEnabledFeatures::setWebGLDraftExtensionsEnabled(enable);
}

void WebRuntimeFeatures::enableWebGLImageChromium(bool enable)
{
    RuntimeEnabledFeatures::setWebGLImageChromiumEnabled(enable);
}

void WebRuntimeFeatures::enableWebMIDI(bool enable)
{
    return RuntimeEnabledFeatures::setWebMIDIEnabled(enable);
}

void WebRuntimeFeatures::enableXSLT(bool enable)
{
    RuntimeEnabledFeatures::setXSLTEnabled(enable);
}

void WebRuntimeFeatures::enableOverlayScrollbars(bool enable)
{
    RuntimeEnabledFeatures::setOverlayScrollbarsEnabled(enable);
}

void WebRuntimeFeatures::enableOverlayFullscreenVideo(bool enable)
{
    RuntimeEnabledFeatures::setOverlayFullscreenVideoEnabled(enable);
}

void WebRuntimeFeatures::enableSharedWorker(bool enable)
{
    RuntimeEnabledFeatures::setSharedWorkerEnabled(enable);
}

void WebRuntimeFeatures::enablePreciseMemoryInfo(bool enable)
{
    RuntimeEnabledFeatures::setPreciseMemoryInfoEnabled(enable);
}

void WebRuntimeFeatures::enableLayerSquashing(bool enable)
{
    RuntimeEnabledFeatures::setLayerSquashingEnabled(enable);
}

void WebRuntimeFeatures::enableShowModalDialog(bool enable)
{
    RuntimeEnabledFeatures::setShowModalDialogEnabled(enable);
}

void WebRuntimeFeatures::enableLaxMixedContentChecking(bool enable)
{
    RuntimeEnabledFeatures::setLaxMixedContentCheckingEnabled(enable);
}

void WebRuntimeFeatures::enableCredentialManagerAPI(bool enable)
{
    RuntimeEnabledFeatures::setCredentialManagerEnabled(enable);
}

void WebRuntimeFeatures::enableTextBlobs(bool enable)
{
    RuntimeEnabledFeatures::setTextBlobEnabled(enable);
}

} // namespace blink
