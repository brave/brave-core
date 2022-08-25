/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SAFE_BUILTINS_RENDERER_SAFE_BUILTINS_HELPERS_H_
#define BRAVE_COMPONENTS_SAFE_BUILTINS_RENDERER_SAFE_BUILTINS_HELPERS_H_

#include <string>

namespace blink {
class WebLocalFrame;
}  // namespace blink

namespace brave {

// Load script in a closure that will use safe builtin types to prevent
// prototype pollution attack. When a new type is added, we need to update
// WrapSource and args for SafeCallFunction.
void LoadScriptWithSafeBuiltins(blink::WebLocalFrame* web_frame,
                                const std::string& script,
                                const std::string& name);

}  //  namespace brave

#endif  // BRAVE_COMPONENTS_SAFE_BUILTINS_RENDERER_SAFE_BUILTINS_HELPERS_H_
