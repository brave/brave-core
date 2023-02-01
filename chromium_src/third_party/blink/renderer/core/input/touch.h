/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_INPUT_TOUCH_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_INPUT_TOUCH_H_

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"

#define screenX                                                                \
  screenX() const {                                                            \
    return brave::FarbledPointerScreenCoordinate(                              \
        target()->ToDOMWindow(), brave::FarbleKey::kPointerScreenX, clientX(), \
        screenX_ChromiumImpl());                                               \
  }                                                                            \
  double screenX_ChromiumImpl

#define screenY                                                                \
  screenY() const {                                                            \
    return brave::FarbledPointerScreenCoordinate(                              \
        target()->ToDOMWindow(), brave::FarbleKey::kPointerScreenY, clientY(), \
        screenY_ChromiumImpl());                                               \
  }                                                                            \
  double screenY_ChromiumImpl

#include "src/third_party/blink/renderer/core/input/touch.h"  // IWYU pragma: export

#undef screenX
#undef screenY

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_INPUT_TOUCH_H_
