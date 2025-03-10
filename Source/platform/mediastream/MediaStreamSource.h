/*
 * Copyright (C) 2011 Ericsson AB. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Ericsson nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#ifndef MediaStreamSource_h
#define MediaStreamSource_h

#include "platform/PlatformExport.h"
#include "platform/audio/AudioDestinationConsumer.h"
#include "platform/heap/Handle.h"
#include "public/platform/WebMediaConstraints.h"
#include "wtf/OwnPtr.h"
#include "wtf/PassOwnPtr.h"
#include "wtf/ThreadingPrimitives.h"
#include "wtf/Vector.h"
#include "wtf/text/WTFString.h"

namespace blink {

class PLATFORM_EXPORT MediaStreamSource FINAL : public GarbageCollectedFinalized<MediaStreamSource> {
public:
    class Observer : public GarbageCollectedMixin {
    public:
        virtual ~Observer() { }
        virtual void sourceChangedState() = 0;
        virtual void trace(Visitor*) { }
    };

    class ExtraData {
    public:
        virtual ~ExtraData() { }
    };

    enum Type {
        TypeAudio,
        TypeVideo
    };

    enum ReadyState {
        ReadyStateLive = 0,
        ReadyStateMuted = 1,
        ReadyStateEnded = 2
    };

    static MediaStreamSource* create(const String& id, Type, const String& name, ReadyState = ReadyStateLive, bool requiresConsumer = false);

    const String& id() const { return m_id; }
    Type type() const { return m_type; }
    const String& name() const { return m_name; }

    void setReadyState(ReadyState);
    ReadyState readyState() const { return m_readyState; }

    void addObserver(Observer*);
    void removeObserver(Observer*);

    ExtraData* extraData() const { return m_extraData.get(); }
    void setExtraData(PassOwnPtr<ExtraData> extraData) { m_extraData = extraData; }

    void setConstraints(blink::WebMediaConstraints constraints) { m_constraints = constraints; }
    blink::WebMediaConstraints constraints() { return m_constraints; }

    void setAudioFormat(size_t numberOfChannels, float sampleRate);
    void consumeAudio(AudioBus*, size_t numberOfFrames);

    bool requiresAudioConsumer() const { return m_requiresConsumer; }
    void addAudioConsumer(AudioDestinationConsumer*);
    bool removeAudioConsumer(AudioDestinationConsumer*);
    const HeapHashSet<Member<AudioDestinationConsumer> >& audioConsumers() { return m_audioConsumers; }

    void trace(Visitor*);
    void dispose();

private:
    MediaStreamSource(const String& id, Type, const String& name, ReadyState, bool requiresConsumer);

    String m_id;
    Type m_type;
    String m_name;
    ReadyState m_readyState;
    bool m_requiresConsumer;
    HeapHashSet<WeakMember<Observer> > m_observers;
    Mutex m_audioConsumersLock;
    HeapHashSet<Member<AudioDestinationConsumer> > m_audioConsumers;
    OwnPtr<ExtraData> m_extraData;
    blink::WebMediaConstraints m_constraints;
};

typedef HeapVector<Member<MediaStreamSource> > MediaStreamSourceVector;

} // namespace blink

#endif // MediaStreamSource_h
