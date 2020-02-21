/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"

#include <algorithm>

#include "brave/components/brave_shields/common/brave_shield_constants.h"

namespace {

const std::vector<std::string> kShieldsResourceIDs {
    brave_shields::kAds,
    brave_shields::kTrackers,
    brave_shields::kHTTPUpgradableResources,
    brave_shields::kJavaScript,
    brave_shields::kFingerprinting,
    brave_shields::kBraveShields,
    brave_shields::kReferrers,
    brave_shields::kCookies };

}  // namespace

namespace content_settings {

const std::vector<std::string>& GetShieldsResourceIDs() {
  return kShieldsResourceIDs;
}

bool IsShieldsResourceID(
    const content_settings::ResourceIdentifier& resource_identifier) {
  return std::find(kShieldsResourceIDs.begin(),
                   kShieldsResourceIDs.end(),
                   resource_identifier) != kShieldsResourceIDs.end();
}

}  // namespace content_settings
