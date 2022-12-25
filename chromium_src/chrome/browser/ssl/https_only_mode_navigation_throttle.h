/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SSL_HTTPS_ONLY_MODE_NAVIGATION_THROTTLE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SSL_HTTPS_ONLY_MODE_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

#define WillFailRequest           \
  WillFailRequest_ChromiumImpl(); \
  content::NavigationThrottle::ThrottleCheckResult WillFailRequest

#include "src/chrome/browser/ssl/https_only_mode_navigation_throttle.h"

#undef WillFailRequest

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SSL_HTTPS_ONLY_MODE_NAVIGATION_THROTTLE_H_
