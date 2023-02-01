/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_POINTER_EVENT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_POINTER_EVENT_H_

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/renderer/core/events/mouse_event.h"

#define screen_x_                        \
  brave::FarbledPointerScreenCoordinate( \
      view(), brave::FarbleKey::kPointerScreenX, client_x_, screen_x_);

#define screen_y_                        \
  brave::FarbledPointerScreenCoordinate( \
      view(), brave::FarbleKey::kPointerScreenY, client_y_, screen_y_);

#include "src/third_party/blink/renderer/core/events/pointer_event.h"  // IWYU pragma: export

#undef screen_x_
#undef screen_y_

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_POINTER_EVENT_H_
