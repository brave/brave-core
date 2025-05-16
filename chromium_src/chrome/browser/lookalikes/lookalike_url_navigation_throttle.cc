/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/lookalikes/lookalike_url_navigation_throttle.h"

#define MaybeCreateAndAdd MaybeCreateAndAdd_ChromiumImpl
#include "src/chrome/browser/lookalikes/lookalike_url_navigation_throttle.cc"
#undef MaybeCreateAndAdd

void LookalikeUrlNavigationThrottle::MaybeCreateAndAdd(
    content::NavigationThrottleRegistry& registry) {}
