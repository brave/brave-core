/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/events/pointer_event_factory.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_mouse_event_init.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"

// Don't trigger a fingerprinting settings check when copying data from an
// existing pointer event to a new pointer event.

#define screenX() screenX_ChromiumImpl()
#define screenY() screenY_ChromiumImpl()

#include "src/third_party/blink/renderer/core/events/pointer_event_factory.cc"

#undef screenX
#undef screenY
