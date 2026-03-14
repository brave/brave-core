/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_HISTORY_BACKEND_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_HISTORY_BACKEND_H_

#define GetHistoryCount  \
  GetKnownToSyncCount(); \
  HistoryCountResult GetHistoryCount

// Override Chromium default history expiration threshold from 90 days to 5 years
// (365 * 5 days) to satisfy user expectations for longer history retention.
// See issue: https://github.com/brave/brave-browser/issues/29045
#define kExpireDaysThreshold      \
  kExpireDaysThreshold = 365 * 5; \
  static constexpr int kExpireDaysThreshold_Chromium

#include <components/history/core/browser/history_backend.h>  // IWYU pragma: export

#undef kExpireDaysThreshold
#undef GetHistoryCount

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_HISTORY_BACKEND_H_
