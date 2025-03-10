// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated by code_generator_v8.py. DO NOT MODIFY!

#ifndef V8TestInterface_h
#define V8TestInterface_h

#if ENABLE(CONDITION)
#include "bindings/core/v8/ScriptWrappable.h"
#include "bindings/core/v8/V8Binding.h"
#include "bindings/core/v8/V8DOMWrapper.h"
#include "bindings/core/v8/V8TestInterfaceEmpty.h"
#include "bindings/core/v8/WrapperTypeInfo.h"
#include "bindings/tests/idls/core/TestInterfaceImplementation.h"
#include "platform/heap/Handle.h"

namespace blink {

class V8TestInterface {
public:
    class PrivateScript {
    public:
        static bool shortMethodWithShortArgumentImplementedInPrivateScriptMethod(LocalFrame* frame, TestInterface* holderImpl, int value, int* result);
        static bool stringAttributeAttributeGetter(LocalFrame* frame, TestInterfaceImplementation* holderImpl, String* result);
        static bool stringAttributeAttributeSetter(LocalFrame* frame, TestInterfaceImplementation* holderImpl, String cppValue);
    };

    static bool hasInstance(v8::Handle<v8::Value>, v8::Isolate*);
    static v8::Handle<v8::Object> findInstanceInPrototypeChain(v8::Handle<v8::Value>, v8::Isolate*);
    static v8::Handle<v8::FunctionTemplate> domTemplate(v8::Isolate*);
    static TestInterfaceImplementation* toImpl(v8::Handle<v8::Object> object)
    {
        return blink::toScriptWrappableBase(object)->toImpl<TestInterfaceImplementation>();
    }
    static TestInterfaceImplementation* toImplWithTypeCheck(v8::Isolate*, v8::Handle<v8::Value>);
    static const WrapperTypeInfo wrapperTypeInfo;
    static void refObject(ScriptWrappableBase* internalPointer);
    static void derefObject(ScriptWrappableBase* internalPointer);
    static WrapperPersistentNode* createPersistentHandle(ScriptWrappableBase* internalPointer);
    static void visitDOMWrapper(ScriptWrappableBase* internalPointer, const v8::Persistent<v8::Object>&, v8::Isolate*);
    static ActiveDOMObject* toActiveDOMObject(v8::Handle<v8::Object>);
    static void implementsCustomVoidMethodMethodCustom(const v8::FunctionCallbackInfo<v8::Value>&);
    static void legacyCallCustom(const v8::FunctionCallbackInfo<v8::Value>&);
    static const int internalFieldCount = v8DefaultWrapperInternalFieldCount + 0;
    static inline ScriptWrappableBase* toScriptWrappableBase(TestInterfaceImplementation* impl)
    {
        return impl->toScriptWrappableBase();
    }
    static void installConditionallyEnabledProperties(v8::Handle<v8::Object>, v8::Isolate*);
    static void installConditionallyEnabledMethods(v8::Handle<v8::Object>, v8::Isolate*);
};

class TestInterfaceImplementation;
v8::Handle<v8::Value> toV8(TestInterfaceImplementation*, v8::Handle<v8::Object> creationContext, v8::Isolate*);

template<class CallbackInfo>
inline void v8SetReturnValue(const CallbackInfo& callbackInfo, TestInterfaceImplementation* impl)
{
    v8SetReturnValue(callbackInfo, toV8(impl, callbackInfo.Holder(), callbackInfo.GetIsolate()));
}

template<class CallbackInfo>
inline void v8SetReturnValueForMainWorld(const CallbackInfo& callbackInfo, TestInterfaceImplementation* impl)
{
     v8SetReturnValue(callbackInfo, toV8(impl, callbackInfo.Holder(), callbackInfo.GetIsolate()));
}

template<class CallbackInfo, class Wrappable>
inline void v8SetReturnValueFast(const CallbackInfo& callbackInfo, TestInterfaceImplementation* impl, Wrappable*)
{
     v8SetReturnValue(callbackInfo, toV8(impl, callbackInfo.Holder(), callbackInfo.GetIsolate()));
}

inline v8::Handle<v8::Value> toV8(PassRefPtr<TestInterfaceImplementation> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl.get(), creationContext, isolate);
}

template<class CallbackInfo>
inline void v8SetReturnValue(const CallbackInfo& callbackInfo, PassRefPtr<TestInterfaceImplementation> impl)
{
    v8SetReturnValue(callbackInfo, impl.get());
}

template<class CallbackInfo>
inline void v8SetReturnValueForMainWorld(const CallbackInfo& callbackInfo, PassRefPtr<TestInterfaceImplementation> impl)
{
    v8SetReturnValueForMainWorld(callbackInfo, impl.get());
}

template<class CallbackInfo, class Wrappable>
inline void v8SetReturnValueFast(const CallbackInfo& callbackInfo, PassRefPtr<TestInterfaceImplementation> impl, Wrappable* wrappable)
{
    v8SetReturnValueFast(callbackInfo, impl.get(), wrappable);
}

} // namespace blink
#endif // ENABLE(CONDITION)

#endif // V8TestInterface_h
