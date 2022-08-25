/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/renderer/v8_helper.h"

#include <utility>

#include "brave/components/safe_builtins/renderer/safe_builtins_helpers.h"
#include "gin/converter.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-microtask-queue.h"
#include "v8/include/v8-object.h"

namespace brave_wallet {

v8::MaybeLocal<v8::Value> GetProperty(v8::Local<v8::Context> context,
                                      v8::Local<v8::Value> object,
                                      const base::StringPiece& name) {
  v8::Local<v8::String> name_str = gin::StringToV8(context->GetIsolate(), name);
  v8::Local<v8::Object> object_obj;
  if (!object->ToObject(context).ToLocal(&object_obj)) {
    return v8::MaybeLocal<v8::Value>();
  }

  return object_obj->Get(context, name_str);
}

v8::Maybe<bool> CreateDataProperty(v8::Local<v8::Context> context,
                                   v8::Local<v8::Object> object,
                                   const base::StringPiece& name,
                                   v8::Local<v8::Value> value) {
  v8::Local<v8::String> name_str = gin::StringToV8(context->GetIsolate(), name);

  return object->CreateDataProperty(context, name_str, value);
}

v8::MaybeLocal<v8::Value> CallMethodOfObject(
    blink::WebLocalFrame* web_frame,
    const base::StringPiece& object_name,
    const base::StringPiece& method_name,
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
    const base::StringPiece& method_name,
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

void ExecuteScript(blink::WebLocalFrame* web_frame,
                   const std::string& script,
                   const std::string& name) {
  if (web_frame->IsProvisional())
    return;

  brave::LoadScriptWithSafeBuiltins(web_frame, script, name);
}

void SetProviderNonWritable(v8::Local<v8::Context> context,
                            v8::Local<v8::Object> global,
                            v8::Local<v8::Value> provider_obj,
                            v8::Local<v8::String> provider_name,
                            bool is_enumerable) {
  v8::PropertyDescriptor desc(provider_obj, false);
  desc.set_configurable(false);
  if (!is_enumerable)
    desc.set_enumerable(false);
  global->DefineProperty(context, provider_name, desc).Check();
}

void SetOwnPropertyNonWritable(v8::Local<v8::Context> context,
                               v8::Local<v8::Object> provider_object,
                               v8::Local<v8::String> property_name) {
  v8::Local<v8::Value> property;
  if (!provider_object->Get(context, property_name).ToLocal(&property))
    return;

  v8::PropertyDescriptor desc(property, false);
  provider_object->DefineProperty(context, property_name, desc).Check();
}

}  // namespace brave_wallet
