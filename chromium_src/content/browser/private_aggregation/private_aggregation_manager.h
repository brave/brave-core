/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_PRIVATE_AGGREGATION_PRIVATE_AGGREGATION_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_PRIVATE_AGGREGATION_PRIVATE_AGGREGATION_MANAGER_H_

// Adding a `GetManager_ChromiumImpl`, so we can replace the existing one as
// nullopt, while keeping the original one off the normal call paths in
// chromium. This way we can just rename the implementation too, and still build
// the TU as expected, rather than shadowing it, dropping the symbols of other
// static functions like `ShouldSendReportDeterministically`.
#define GetManager(...)                 \
  GetManager_ChromiumImpl(__VA_ARGS__); \
  static PrivateAggregationManager* GetManager(__VA_ARGS__)
#include "src/content/browser/private_aggregation/private_aggregation_manager.h"  // IWYU pragma: export
#undef GetManager

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_PRIVATE_AGGREGATION_PRIVATE_AGGREGATION_MANAGER_H_
