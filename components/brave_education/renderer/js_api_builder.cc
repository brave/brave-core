// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_education/renderer/js_api_builder.h"

namespace brave_education {

JSAPIBuilder JSAPIBuilder::Create(v8::Isolate* isolate,
                                  v8::Local<v8::Context> context) {
  return JSAPIBuilder(isolate, context);
}

JSAPIBuilder::JSAPIBuilder(v8::Isolate* isolate, v8::Local<v8::Context> context)
    : isolate_(isolate), context_(context), handle_scope_(isolate) {
  CHECK(isolate_);
  object_ = v8::Object::New(isolate_);
}

JSAPIBuilder::~JSAPIBuilder() = default;

void JSAPIBuilder::SetAsObjectProperty(v8::Local<v8::Object> object,
                                       const std::string& name) {
  object->Set(context_, gin::StringToV8(isolate_, name), object_).Check();
}

}  // namespace brave_education
