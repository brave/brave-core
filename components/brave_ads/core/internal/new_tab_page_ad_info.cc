/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/new_tab_page_ad_info.h"

#include <tuple>

namespace brave_ads {

NewTabPageAdInfo::NewTabPageAdInfo() = default;

NewTabPageAdInfo::NewTabPageAdInfo(const NewTabPageAdInfo& other) = default;

NewTabPageAdInfo& NewTabPageAdInfo::operator=(const NewTabPageAdInfo& other) =
    default;

NewTabPageAdInfo::NewTabPageAdInfo(NewTabPageAdInfo&& other) noexcept = default;

NewTabPageAdInfo& NewTabPageAdInfo::operator=(
    NewTabPageAdInfo&& other) noexcept = default;

NewTabPageAdInfo::~NewTabPageAdInfo() = default;

bool NewTabPageAdInfo::operator==(const NewTabPageAdInfo& other) const {
  if (!AdInfo::operator==(other)) {
    return false;
  }

  auto tie = [](const NewTabPageAdInfo& ad) {
    return std::tie(ad.company_name, ad.image_url, ad.alt, ad.wallpapers);
  };

  return tie(*this) == tie(other);
}

bool NewTabPageAdInfo::operator!=(const NewTabPageAdInfo& other) const {
  return !(*this == other);
}

bool NewTabPageAdInfo::IsValid() const {
  return AdInfo::IsValid() && !company_name.empty() && image_url.is_valid() &&
         !alt.empty() && !wallpapers.empty();
}

bool NewTabPageAdInfo::HasValidCreativeInstanceIdAndCampaignId() const {
  return !creative_instance_id.empty() && !campaign_id.empty();
}

}  // namespace brave_ads
