/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/enterprise/platform_auth/platform_auth_navigation_throttle.h"

namespace content {
class NavigationThrottleRegistry;
}  // namespace content

namespace brave {

// Implemented in brave/browser/enterprise/platform_auth_helper.cc
bool ShouldSkipPlatformAuth(content::NavigationThrottleRegistry& registry);

}  // namespace brave

#include <chrome/browser/enterprise/platform_auth/platform_auth_navigation_throttle.cc>
