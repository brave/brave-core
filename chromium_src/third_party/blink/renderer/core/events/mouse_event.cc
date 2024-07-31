/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/events/mouse_event.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_mouse_event_init.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"

// Replace screen{X,Y} with screen{X,Y}_ChromiumImpl so we don't trigger
// an unnecessary fingerprint settings check on creating a double-click event.
#define screenX() screenX_ChromiumImpl()
#define screenY() screenY_ChromiumImpl()

#include "src/third_party/blink/renderer/core/events/mouse_event.cc"

#undef screenX
#undef screenY
