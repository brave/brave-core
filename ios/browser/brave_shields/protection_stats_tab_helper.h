// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_PROTECTION_STATS_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_PROTECTION_STATS_TAB_HELPER_H_

#import <Foundation/Foundation.h>

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "ios/web/public/web_state_user_data.h"

@protocol ProtectionStatsTabHelperBridge;

class GURL;

namespace web {
class WebState;
}  // namespace web

namespace brave_shields {

struct BlockedResource {
  std::string resource_url;
  std::string resource_type;
};

// Tab helper that bridges blocked resource reports from the
// ProtectionStatsJavaScriptFeature to the platform Shields stats handler.
class ProtectionStatsTabHelper
    : public web::WebStateUserData<ProtectionStatsTabHelper> {
 public:
  ~ProtectionStatsTabHelper() override;

  void SetBridge(id<ProtectionStatsTabHelperBridge> bridge);

  // Forwards the resources reported by `frame_origin` to the bridge.
  // `resources` is a list of maps each containing a "resourceURL" and a
  // "resourceType" key.
  void HandleBlockedResources(const std::vector<BlockedResource>& resources,
                              const GURL& frame_origin);

 private:
  friend class web::WebStateUserData<ProtectionStatsTabHelper>;

  explicit ProtectionStatsTabHelper(web::WebState* web_state);

  __weak id<ProtectionStatsTabHelperBridge> bridge_;
};

}  // namespace brave_shields

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_PROTECTION_STATS_TAB_HELPER_H_
