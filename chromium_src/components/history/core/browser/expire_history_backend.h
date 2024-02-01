/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_EXPIRE_HISTORY_BACKEND_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_EXPIRE_HISTORY_BACKEND_H_

#define ScheduleExpire                                                      \
  ScheduleExpire();                                                         \
  FRIEND_TEST_ALL_PREFIXES(BraveHistoryBackendTest, ExpireDaysThreshold60); \
  void ScheduleExpireUnused

#include "src/components/history/core/browser/expire_history_backend.h"  // IWYU pragma: export

#undef ScheduleExpire

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_EXPIRE_HISTORY_BACKEND_H_
