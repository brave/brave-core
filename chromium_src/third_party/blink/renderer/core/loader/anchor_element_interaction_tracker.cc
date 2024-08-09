/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/loader/anchor_element_interaction_tracker.h"

#include "third_party/blink/public/common/input/web_mouse_wheel_event.h"
#include "third_party/blink/renderer/core/events/pointer_event.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/screen.h"

// Don't trigger a fingerprinting settings check, because the interaction
// tracker does not leak screenY to content.
#define screenY() screenY_ChromiumImpl()

#include "src/third_party/blink/renderer/core/loader/anchor_element_interaction_tracker.cc"

#undef screenY
