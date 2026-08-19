/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/containers/buildflags/buildflags.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle_registry.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

namespace brave {

bool ShouldSkipPlatformAuth(content::NavigationThrottleRegistry& registry) {
#if BUILDFLAG(ENABLE_CONTAINERS)
  // Container tabs share a normal profile (not OTR), so without this guard
  // PlatformAuthNavigationThrottle would inject device-account identity across
  // isolated container storage partitions.
  if (base::FeatureList::IsEnabled(containers::features::kContainers)) {
    content::WebContents* web_contents =
        registry.GetNavigationHandle().GetWebContents();
    return !containers::GetContainerIdForWebContents(web_contents).empty();
  }
#endif  // BUILDFLAG(ENABLE_CONTAINERS)
  return false;
}

}  // namespace brave
