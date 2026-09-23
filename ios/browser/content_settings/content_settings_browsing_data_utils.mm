// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#import "brave/ios/browser/content_settings/content_settings_browsing_data_utils.h"

#import <Foundation/Foundation.h>

#include "base/apple/foundation_util.h"
#include "base/time/time.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_browsing_data_utils.h"
#include "brave/ios/browser/api/profile/profile_bridge_impl.h"
#include "ios/chrome/browser/content_settings/model/host_content_settings_map_factory.h"

void BraveRemoveSiteSettingsData(NSDate* delete_begin,
                                 NSDate* delete_end,
                                 id<ProfileBridge> profile) {
  ProfileBridgeImpl* holder =
      base::apple::ObjCCastStrict<ProfileBridgeImpl>(profile);
  auto* host_content_settings_map =
      ios::HostContentSettingsMapFactory::GetForProfile(holder.profile);
  CHECK(host_content_settings_map);

  browsing_data::BraveRemoveSiteSettingsData(
      base::Time::FromNSDate(delete_begin), base::Time::FromNSDate(delete_end),
      host_content_settings_map);
}
