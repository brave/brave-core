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
    const std::vector<base::flat_map<std::string, std::string>>& resources,
    const GURL& frame_origin) {
  if (!bridge_) {
    return;
  }

  NSMutableArray<NSDictionary<NSString*, NSString*>*>* ns_resources =
      [[NSMutableArray alloc] initWithCapacity:resources.size()];
  for (const auto& resource : resources) {
    NSMutableDictionary<NSString*, NSString*>* ns_resource =
        [[NSMutableDictionary alloc] initWithCapacity:resource.size()];
    for (const auto& [key, value] : resource) {
      ns_resource[base::SysUTF8ToNSString(key)] =
          base::SysUTF8ToNSString(value);
    }
    [ns_resources addObject:ns_resource];
  }

  [bridge_ handleBlockedResources:ns_resources
                   securityOrigin:net::NSURLWithGURL(frame_origin)];
}

}  // namespace brave_shields
