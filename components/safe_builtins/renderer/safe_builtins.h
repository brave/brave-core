/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SAFE_BUILTINS_RENDERER_SAFE_BUILTINS_H_
#define BRAVE_COMPONENTS_SAFE_BUILTINS_RENDERER_SAFE_BUILTINS_H_

#include <memory>

#include "base/memory/stack_allocated.h"
#include "v8/include/v8-forward.h"
#include "v8/include/v8-persistent-handle.h"

namespace brave {

// This is originated from extensions::SafeBuiltins
// see //extensions/renderer/safe_builtins.h for details
class SafeBuiltins {
  STACK_ALLOCATED();

 public:
  // Creates the v8::Extension which manages SafeBuiltins instances.
  static std::unique_ptr<v8::Extension> CreateV8Extension();

  explicit SafeBuiltins(const v8::Local<v8::Context>& context);

  SafeBuiltins(const SafeBuiltins&) = delete;
  SafeBuiltins& operator=(const SafeBuiltins&) = delete;

  ~SafeBuiltins();

  // We only need safe Object for scripts in
  // //brave/components/brave_wallt/resources for now.
  // see //extensions/renderer/safe_builtins.h for reason of the naming.
  v8::Local<v8::Object> GetObjekt() const;
  v8::Local<v8::Object> GetFunction() const;
  // This is only used as compatibility of iOS overwrite.
  v8::Local<v8::Object> GetFunctionOverride() const;
  v8::Local<v8::Object> GetArray() const;
  // Add more safe builtins if needed.

 private:
  v8::Global<v8::Context> context_;
  v8::Isolate* isolate_;
};

}  //  namespace brave

#endif  // BRAVE_COMPONENTS_SAFE_BUILTINS_RENDERER_SAFE_BUILTINS_H_
