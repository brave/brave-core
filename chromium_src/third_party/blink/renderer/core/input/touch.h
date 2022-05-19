/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_INPUT_TOUCH_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_INPUT_TOUCH_H_

#include "third_party/blink/renderer/core/frame/local_dom_window.h"

#define screenX                                                            \
  screenX() const {                                                        \
    auto* local_dom_window = target()->ToLocalDOMWindow();                 \
    int delta_screenX = local_dom_window                                   \
                            ? local_dom_window->screenX() -                \
                                  local_dom_window->screenX_ChromiumImpl() \
                            : 0;                                           \
    int delta_outerWidth =                                                 \
        local_dom_window ? local_dom_window->outerWidth() -                \
                               local_dom_window->outerWidth_ChromiumImpl() \
                         : 0;                                              \
    return std::floor(screenX_ChromiumImpl() + delta_screenX +             \
                      delta_outerWidth);                                   \
  }                                                                        \
  double screenX_ChromiumImpl

#define screenY                                                             \
  screenY() const {                                                         \
    auto* local_dom_window = target()->ToLocalDOMWindow();                  \
    int delta_screenY = local_dom_window                                    \
                            ? local_dom_window->screenY() -                 \
                                  local_dom_window->screenY_ChromiumImpl()  \
                            : 0;                                            \
    int delta_outerHeight =                                                 \
        local_dom_window ? local_dom_window->outerHeight() -                \
                               local_dom_window->outerHeight_ChromiumImpl() \
                         : 0;                                               \
    return std::floor(screenY_ChromiumImpl() + delta_screenY +              \
                      delta_outerHeight);                                   \
  }                                                                         \
  double screenY_ChromiumImpl

#include "src/third_party/blink/renderer/core/input/touch.h"

#undef screenX
#undef screenY

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_INPUT_TOUCH_H_
