/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/blink_converters.h"

#include <memory>

#include "base/no_destructor.h"
#include "brave/v8/include/v8-isolate-page-graph-utils.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_v8_value_converter.h"
#include "third_party/blink/renderer/core/dom/document.h"

namespace blink {

namespace {

// Serializes v8::Value using Inspector Protocol internals.
base::Value V8ValueToPageGraphValue(v8::Isolate* isolate,
                                    v8::Local<v8::Value> v8_value) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Value> serialized_value =
      v8::page_graph::SerializeValue(context, v8_value);
  if (!serialized_value.IsEmpty()) {
    static base::NoDestructor<std::unique_ptr<WebV8ValueConverter>> converter(
        Platform::Current()->CreateWebV8ValueConverter());
    auto value =
        (*converter)
            ->FromV8Value(serialized_value, isolate->GetCurrentContext());
    if (value) {
      return std::move(*value);
    }
  }
  return {};
}

}  // namespace

template <>
base::Value ToPageGraphValue(
    ScriptState* script_state,
    const bindings::NativeValueTraitsAnyAdapter& adapter) {
  return ToPageGraphValue(script_state, static_cast<ScriptValue>(adapter));
}

template <>
base::Value ToPageGraphValue(ScriptState* script_state,
                             const v8::Local<v8::Value>& arg) {
  if (arg.IsEmpty()) {
    return base::Value();
  }
  v8::String::Utf8Value utf8_string(script_state->GetIsolate(), arg);
  std::string_view result(*utf8_string, utf8_string.length());
  // If the value is converted to string as [object something], we need to
  // serialize it using the inspector protocol.
  if (result.starts_with("[object ")) {
    return V8ValueToPageGraphValue(script_state->GetIsolate(), arg);
  }
  return base::Value(result);
}

template <>
base::Value ToPageGraphValue(ScriptState* script_state,
                             const ScriptValue& script_value) {
  v8::Local<v8::Value> value = script_value.V8Value();
  return ToPageGraphValue(script_state, value);
}

template <>
base::Value ToPageGraphValue(ScriptState* script_state,
                             const ScriptPromiseUntyped& script_promise) {
  return ToPageGraphValue(
      script_state,
      ScriptValue(script_state->GetIsolate(), script_promise.V8Promise()));
}

template <>
base::Value ToPageGraphValue(ScriptState* script_state,
                             blink::EventListener* const& event_listener) {
  return event_listener ? base::Value(event_listener->ScriptBody().Utf8())
                        : base::Value();
}

template <>
PageGraphObject ToPageGraphObject(Document* document) {
  return PageGraphObject().Set(
      "cookie_url", base::Value(document->CookieURL().GetString().Utf8()));
}

std::optional<base::AutoReset<bool>> ScopedPageGraphCall() {
  thread_local static bool in_page_graph_call = false;
  if (in_page_graph_call) {
    return std::nullopt;
  }
  return base::AutoReset<bool>(&in_page_graph_call, true);
}

}  // namespace blink
