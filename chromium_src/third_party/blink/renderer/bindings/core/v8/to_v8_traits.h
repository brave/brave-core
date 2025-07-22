/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_TO_V8_TRAITS_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_TO_V8_TRAITS_H_

#define BRAVE_TO_V8_TRAITS_PAGE_GRAPH_OVERRIDE                                 \
  template <typename ContainerType>                                            \
  [[nodiscard]] static v8::Local<v8::Value> ToV8(ScriptState* script_state,    \
                                                 const ContainerType* value) { \
    if (!value)                                                                \
      return v8::Null(script_state->GetIsolate());                             \
    return ToV8Traits<IDLSequence<T>>::ToV8(script_state, *value);             \
  }

#include "src/third_party/blink/renderer/bindings/core/v8/to_v8_traits.h"  // IWYU pragma: export

#undef BRAVE_TO_V8_TRAITS_PAGE_GRAPH_OVERRIDE

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_TO_V8_TRAITS_H_
