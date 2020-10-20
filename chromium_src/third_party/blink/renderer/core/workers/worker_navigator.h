/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_NAVIGATOR_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_NAVIGATOR_H_

#include "third_party/blink/renderer/core/frame/navigator_id.h"

#define userAgent                 \
  userAgent_ChromiumImpl() const; \
  String userAgent

#include "../../../../../../../third_party/blink/renderer/core/workers/worker_navigator.h"

#undef userAgent

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_WORKERS_WORKER_NAVIGATOR_H_
