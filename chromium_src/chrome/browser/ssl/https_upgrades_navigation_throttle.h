/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SSL_HTTPS_UPGRADES_NAVIGATION_THROTTLE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SSL_HTTPS_UPGRADES_NAVIGATION_THROTTLE_H_

#define MaybeCreateThrottleFor(...)                       \
  MaybeCreateThrottleFor_ChromiumImpl(__VA_ARGS__);       \
  static std::unique_ptr<HttpsUpgradesNavigationThrottle> \
  MaybeCreateThrottleFor(__VA_ARGS__)

#include "src/chrome/browser/ssl/https_upgrades_navigation_throttle.h"  // IWYU pragma: export

#undef MaybeCreateThrottleFor

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SSL_HTTPS_UPGRADES_NAVIGATION_THROTTLE_H_
