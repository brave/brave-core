/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_REPORTING_OBSERVER_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_REPORTING_OBSERVER_H_

#define QueueReport                   \
  QueueReport_Unused(Report* report); \
  void QueueReport

#include "src/third_party/blink/renderer/core/frame/reporting_observer.h"  // IWYU pragma: export

#undef QueueReport

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_REPORTING_OBSERVER_H_
