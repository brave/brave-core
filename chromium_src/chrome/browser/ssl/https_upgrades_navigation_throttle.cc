/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ssl/https_upgrades_navigation_throttle.h"

#include "base/time/time.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

namespace {

// Tor is slow and needs a longer fallback delay
constexpr base::TimeDelta kTorFallbackDelay = base::Seconds(20);

bool IsTor(content::NavigationHandle* handle) {
  auto* context = handle->GetWebContents()->GetBrowserContext();
  Profile* profile = Profile::FromBrowserContext(context);
  return profile->IsTor();
}

}  // namespace

#define SetNavigationTimeout(DEFAULT_TIMEOUT)                         \
  SetNavigationTimeout(IsTor(navigation_handle()) ? kTorFallbackDelay \
                                                  : DEFAULT_TIMEOUT)

#include <chrome/browser/ssl/https_upgrades_navigation_throttle.cc>

#undef SetNavigationTimeout
