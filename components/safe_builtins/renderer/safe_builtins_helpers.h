/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SAFE_BUILTINS_RENDERER_SAFE_BUILTINS_HELPERS_H_
#define BRAVE_COMPONENTS_SAFE_BUILTINS_RENDERER_SAFE_BUILTINS_HELPERS_H_

#include <string>

#include "v8/include/v8-local-handle.h"
#include "v8/include/v8-value.h"

namespace blink {
class WebLocalFrame;
}  // namespace blink

namespace brave {

// Load script in a closure that will use safe builtin types to prevent
// prototype pollution attack. When a new type is added, we need to update
// WrapSource and args for SafeCallFunction.
v8::MaybeLocal<v8::Value> LoadScriptWithSafeBuiltins(
    blink::WebLocalFrame* web_frame,
    const std::string& script);

}  //  namespace brave

#endif  // BRAVE_COMPONENTS_SAFE_BUILTINS_RENDERER_SAFE_BUILTINS_HELPERS_H_
