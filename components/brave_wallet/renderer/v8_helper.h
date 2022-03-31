/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_V8_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_V8_HELPER_H_

#include <string>
#include <vector>

#include "v8/include/v8-context.h"
#include "v8/include/v8-local-handle.h"
#include "v8/include/v8-value.h"

namespace blink {
class WebLocalFrame;
}  // namespace blink

namespace brave_wallet {

v8::MaybeLocal<v8::Value> GetProperty(v8::Local<v8::Context> context,
                                      v8::Local<v8::Value> object,
                                      const std::u16string& name);
v8::Maybe<bool> CreateDataProperty(v8::Local<v8::Context> context,
                                   v8::Local<v8::Object> object,
                                   const std::u16string& name,
                                   v8::Local<v8::Value> value);

v8::MaybeLocal<v8::Value> CallMethodOfObject(
    blink::WebLocalFrame* web_frame,
    const std::u16string& object_name,
    const std::u16string& method_name,
    std::vector<v8::Local<v8::Value>>&& args);

v8::MaybeLocal<v8::Value> CallMethodOfObject(
    blink::WebLocalFrame* web_frame,
    v8::Local<v8::Value> object,
    const std::u16string& method_name,
    std::vector<v8::Local<v8::Value>>&& args);

void ExecuteScript(blink::WebLocalFrame* web_frame, const std::string script);

// By default we allow extensions to overwrite the window.[provider] object
// but if the user goes into settings and explicitly selects to use Brave Wallet
// then we will block modifications to window.[provider] here.
void SetProviderNonWritable(blink::WebLocalFrame* web_frame,
                            const std::string& provider);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_RENDERER_V8_HELPER_H_
