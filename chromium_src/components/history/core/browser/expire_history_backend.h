/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_EXPIRE_HISTORY_BACKEND_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_EXPIRE_HISTORY_BACKEND_H_

#define ClearOldOnDemandFaviconsIfPossible(...)    \
  ClearOldOnDemandFaviconsIfPossible(__VA_ARGS__); \
  void UpdateExpirationThreshold(base::TimeDelta threshold)

#include <components/history/core/browser/expire_history_backend.h>  // IWYU pragma: export

#undef ClearOldOnDemandFaviconsIfPossible

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_HISTORY_CORE_BROWSER_EXPIRE_HISTORY_BACKEND_H_
