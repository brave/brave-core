/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_info.h"

namespace ads {

AdInfo::AdInfo() = default;
AdInfo::AdInfo(const AdInfo& info) = default;
AdInfo::~AdInfo() = default;

bool AdInfo::operator==(const AdInfo& rhs) const {
  return type == rhs.type && uuid == rhs.uuid &&
         creative_instance_id == rhs.creative_instance_id &&
         creative_set_id == rhs.creative_set_id &&
         campaign_id == rhs.campaign_id && advertiser_id == rhs.advertiser_id &&
         segment == rhs.segment && target_url == rhs.target_url;
}

bool AdInfo::operator!=(const AdInfo& rhs) const {
  return !(*this == rhs);
}

bool AdInfo::IsValid() const {
  if (type == AdType::kUndefined || creative_instance_id.empty() ||
      creative_set_id.empty() || campaign_id.empty() || segment.empty() ||
      target_url.empty()) {
    return false;
  }

  return true;
}

}  // namespace ads
