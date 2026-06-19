// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/protection_stats_tab_helper.h"

#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/brave_shields/protection_stats_tab_helper_bridge.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

namespace brave_shields {

ProtectionStatsTabHelper::ProtectionStatsTabHelper(web::WebState* web_state) {}

ProtectionStatsTabHelper::~ProtectionStatsTabHelper() = default;

void ProtectionStatsTabHelper::SetBridge(
    id<ProtectionStatsTabHelperBridge> bridge) {
  bridge_ = bridge;
}

void ProtectionStatsTabHelper::HandleBlockedResources(
    const std::vector<BlockedResource>& resources,
    const GURL& frame_origin) {
  if (!bridge_) {
    return;
  }

  NSMutableArray<ProtectionStatsResource*>* ns_resources =
      [[NSMutableArray alloc] initWithCapacity:resources.size()];
  for (const auto& resource : resources) {
    [ns_resources
        addObject:[[ProtectionStatsResource alloc]
                      initWithURL:base::SysUTF8ToNSString(resource.resource_url)
                             type:base::SysUTF8ToNSString(
                                      resource.resource_type)]];
  }

  [bridge_ handleBlockedResources:ns_resources
                   securityOrigin:net::NSURLWithGURL(frame_origin)];
}

}  // namespace brave_shields
