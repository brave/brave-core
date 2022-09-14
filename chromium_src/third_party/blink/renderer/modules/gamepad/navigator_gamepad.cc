/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/gamepad/navigator_gamepad.h"

#include "base/trace_event/trace_event_impl.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/modules/gamepad/gamepad_dispatcher.h"

#define timestamp() timestamp().GetValue()

#include "src/third_party/blink/renderer/modules/gamepad/navigator_gamepad.cc"

#undef timestamp
