/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/lookalikes/lookalike_url_navigation_throttle.h"

#define MaybeCreateNavigationThrottle MaybeCreateNavigationThrottle_ChromiumImpl
#include "../../../../../chrome/browser/lookalikes/lookalike_url_navigation_throttle.cc"
#undef MaybeCreateNavigationThrottle

std::unique_ptr<LookalikeUrlNavigationThrottle>
LookalikeUrlNavigationThrottle::MaybeCreateNavigationThrottle(
    content::NavigationHandle* navigation_handle) {
  return nullptr;
}
