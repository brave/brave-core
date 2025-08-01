/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_VISIT_DATABASE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_VISIT_DATABASE_H_

#define GetHistoryCount            \
  GetKnownToSyncCount(int* count); \
  bool GetHistoryCount

#include <components/history/core/browser/visit_database.h>  // IWYU pragma: export

#undef GetHistoryCount

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_VISIT_DATABASE_H_
