/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/v8_helper.h"

#include <utility>

#include "base/strings/stringprintf.h"
#include "gin/converter.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-microtask-queue.h"

namespace brave_wallet {

v8::MaybeLocal<v8::Value> GetProperty(v8::Local<v8::Context> context,
                                      v8::Local<v8::Value> object,
                                      const std::u16string& name) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::String> name_str =
      gin::ConvertToV8(isolate, name).As<v8::String>();
  v8::Local<v8::Object> object_obj;
  if (!object->ToObject(context).ToLocal(&object_obj)) {
    return v8::MaybeLocal<v8::Value>();
  }

  return object_obj->Get(context, name_str);
}

v8::Maybe<bool> CreateDataProperty(v8::Local<v8::Context> context,
                                   v8::Local<v8::Object> object,
                                   const std::u16string& name,
                                   v8::Local<v8::Value> value) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::Local<v8::String> name_str =
      gin::ConvertToV8(isolate, name).As<v8::String>();

  return object->CreateDataProperty(context, name_str, value);
}

v8::MaybeLocal<v8::Value> CallMethodOfObject(
    blink::WebLocalFrame* web_frame,
    const std::u16string& object_name,
    const std::u16string& method_name,
    std::vector<v8::Local<v8::Value>>&& args) {
  if (web_frame->IsProvisional())
    return v8::Local<v8::Value>();
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = web_frame->MainWorldScriptContext();
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Value> object;
  if (!GetProperty(context, context->Global(), object_name).ToLocal(&object)) {
    return v8::Local<v8::Value>();
  }

  return CallMethodOfObject(web_frame, object, method_name, std::move(args));
}

v8::MaybeLocal<v8::Value> CallMethodOfObject(
    blink::WebLocalFrame* web_frame,
    v8::Local<v8::Value> object,
    const std::u16string& method_name,
    std::vector<v8::Local<v8::Value>>&& args) {
  if (web_frame->IsProvisional())
    return v8::Local<v8::Value>();
  v8::Local<v8::Context> context = web_frame->MainWorldScriptContext();
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Context::Scope context_scope(context);
  v8::MicrotasksScope microtasks(isolate,
                                 v8::MicrotasksScope::kDoNotRunMicrotasks);
  v8::Local<v8::Value> method;
  if (!GetProperty(context, object, method_name).ToLocal(&method)) {
    return v8::Local<v8::Value>();
  }

  // Without the IsFunction test here JS blocking from content settings
  // will trigger a DCHECK crash.
  if (method.IsEmpty() || !method->IsFunction()) {
    return v8::Local<v8::Value>();
  }

  return web_frame->CallFunctionEvenIfScriptDisabled(
      v8::Local<v8::Function>::Cast(method), object,
      static_cast<int>(args.size()), args.data());
}

void ExecuteScript(blink::WebLocalFrame* web_frame, const std::string script) {
  if (web_frame->IsProvisional())
    return;

  web_frame->ExecuteScript(
      blink::WebScriptSource(blink::WebString::FromUTF8(script)));
}

void SetProviderNonWritable(blink::WebLocalFrame* web_frame,
                            const std::string& provider) {
  const char* provider_str = provider.c_str();
  const std::string script = base::StringPrintf(
      R"(;(function() {
           Object.defineProperty(window, '%s', {
             value: window.%s,
             configurable: false,
             writable: false
           });
    })();)",
      provider_str, provider_str);
  ExecuteScript(web_frame, script);
}

}  // namespace brave_wallet
