/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_info.h"

namespace ads {

AdInfo::AdInfo() = default;

AdInfo::AdInfo(const AdInfo& info) = default;

AdInfo& AdInfo::operator=(const AdInfo& info) = default;

AdInfo::~AdInfo() = default;

bool AdInfo::operator==(const AdInfo& rhs) const {
  auto tie = [](const AdInfo& ad) {
    return std::tie(ad.type, ad.placement_id, ad.creative_instance_id,
                    ad.creative_set_id, ad.campaign_id, ad.advertiser_id,
                    ad.segment, ad.target_url);
  };

  return tie(*this) == tie(rhs);
}

bool AdInfo::operator!=(const AdInfo& rhs) const {
  return !(*this == rhs);
}

bool AdInfo::IsValid() const {
  return !(type == AdType::kUndefined || placement_id.empty() ||
           creative_instance_id.empty() || creative_set_id.empty() ||
           campaign_id.empty() || advertiser_id.empty() || segment.empty() ||
           !target_url.is_valid());
}

}  // namespace ads
