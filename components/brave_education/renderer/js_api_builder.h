// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_EDUCATION_RENDERER_JS_API_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_EDUCATION_RENDERER_JS_API_BUILDER_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "gin/converter.h"
#include "gin/function_template.h"
#include "v8/include/v8.h"

namespace brave_education {

class JSAPIBuilder {
 public:
  static JSAPIBuilder Create(v8::Isolate* isolate,
                             v8::Local<v8::Context> context);

  JSAPIBuilder(v8::Isolate* isolate, v8::Local<v8::Context> context);

  JSAPIBuilder(const JSAPIBuilder&) = delete;
  JSAPIBuilder& operator=(const JSAPIBuilder&) = delete;

  ~JSAPIBuilder();

  template <typename T>
  JSAPIBuilder& SetMethod(const std::string& name, const T& callback) {
    auto func = gin::CreateFunctionTemplate(isolate_, callback)
                    ->GetFunction(context_)
                    .ToLocalChecked();

    object_->Set(context_, gin::StringToSymbol(isolate_, name), func).Check();
    return *this;
  }

  void SetAsObjectProperty(v8::Local<v8::Object> object,
                           const std::string& name);

  v8::Local<v8::Object> object() { return object_; }

 private:
  v8::Isolate* isolate_;
  v8::Local<v8::Context> context_;
  v8::HandleScope handle_scope_;
  v8::Local<v8::Object> object_;
};

}  // namespace brave_education

#endif  // BRAVE_COMPONENTS_BRAVE_EDUCATION_RENDERER_JS_API_BUILDER_H_
