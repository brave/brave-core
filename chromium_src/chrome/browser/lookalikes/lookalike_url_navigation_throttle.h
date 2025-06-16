/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_LOOKALIKES_LOOKALIKE_URL_NAVIGATION_THROTTLE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_LOOKALIKES_LOOKALIKE_URL_NAVIGATION_THROTTLE_H_

#define MaybeCreateAndAdd                                           \
  MaybeCreateAndAdd(content::NavigationThrottleRegistry& registry); \
  static void MaybeCreateAndAdd_ChromiumImpl

#include "src/chrome/browser/lookalikes/lookalike_url_navigation_throttle.h"  // IWYU pragma: export
#undef MaybeCreateAndAdd

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_LOOKALIKES_LOOKALIKE_URL_NAVIGATION_THROTTLE_H_
