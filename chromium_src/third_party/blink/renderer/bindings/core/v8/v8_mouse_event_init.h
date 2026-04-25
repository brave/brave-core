/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_MOUSE_EVENT_INIT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_MOUSE_EVENT_INIT_H_

// Alias screenX as screenX_ChromiumImpl so we can freely redefine screenX to
// screenX_ChromiumImpl in mouse_event.cc. Same for screenY.

#define screenX()                \
  screenX_ChromiumImpl() const { \
    return screenX();            \
  }                              \
  double screenX()

#define screenY()                \
  screenY_ChromiumImpl() const { \
    return screenY();            \
  }                              \
  double screenY()

#include "../gen/third_party/blink/renderer/bindings/core/v8/v8_mouse_event_init.h"  // IWYU pragma: export

#undef screenX
#undef screenY

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_MOUSE_EVENT_INIT_H_
