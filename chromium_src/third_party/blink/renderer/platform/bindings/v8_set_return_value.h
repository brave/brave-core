/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_V8_SET_RETURN_VALUE_H_

#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_V8_SET_RETURN_VALUE_H_

#include "src/third_party/blink/renderer/platform/bindings/v8_set_return_value.h"

namespace blink {

namespace bindings {

template <typename CallbackInfo>
inline void V8SetReturnValue(const CallbackInfo& info,
                             double value,
                             V8ReturnValue::PrimitiveType<double>) {
  V8SetReturnValue(info, static_cast<double>(value));
}

}  // namespace bindings

}  // namespace blink

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_V8_SET_RETURN_VALUE_H_
