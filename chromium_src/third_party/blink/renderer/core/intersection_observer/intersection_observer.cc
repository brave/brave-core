/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/intersection_observer/intersection_observer.h"

// Some internal code is expecting double instead of DOMHighResTimeStamp.
// Replacing it here doesn't affect the type exposed to content.
#define DOMHighResTimeStamp double

#include "src/third_party/blink/renderer/core/intersection_observer/intersection_observer.cc"

#undef DOMHighResTimeStamp
