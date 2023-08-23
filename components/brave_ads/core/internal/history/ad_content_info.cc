/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/history/ad_content_info.h"

#include <tuple>

namespace brave_ads {

AdContentInfo::AdContentInfo() = default;

AdContentInfo::AdContentInfo(const AdContentInfo& other) = default;

AdContentInfo& AdContentInfo::operator=(const AdContentInfo& other) = default;

AdContentInfo::AdContentInfo(AdContentInfo&& other) noexcept = default;

AdContentInfo& AdContentInfo::operator=(AdContentInfo&& other) noexcept =
    default;

AdContentInfo::~AdContentInfo() = default;

bool operator==(const AdContentInfo& lhs, const AdContentInfo& rhs) {
  const auto tie = [](const AdContentInfo& ad_content) {
    return std::tie(ad_content.type, ad_content.placement_id,
                    ad_content.creative_instance_id, ad_content.creative_set_id,
                    ad_content.campaign_id, ad_content.advertiser_id,
                    ad_content.brand, ad_content.brand_info,
                    ad_content.brand_display_url, ad_content.brand_url,
                    ad_content.user_reaction_type, ad_content.confirmation_type,
                    ad_content.is_saved, ad_content.is_flagged);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const AdContentInfo& lhs, const AdContentInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
