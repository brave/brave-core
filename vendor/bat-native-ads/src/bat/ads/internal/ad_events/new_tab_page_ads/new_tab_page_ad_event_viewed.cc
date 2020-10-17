/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/new_tab_page_ads/new_tab_page_ad_event_viewed.h"

#include <utility>

#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"
#include "bat/ads/internal/frequency_capping/ad_exclusion_rules/new_tab_page_ad_wallpaper_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/new_tab_page_ads_per_day_frequency_cap.h"
#include "bat/ads/internal/frequency_capping/permission_rules/new_tab_page_ads_per_hour_frequency_cap.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

namespace {
const ConfirmationType kConfirmationType = ConfirmationType::kViewed;
}  // namespace

NewTabPageAdEventViewed::NewTabPageAdEventViewed(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

NewTabPageAdEventViewed::~NewTabPageAdEventViewed() = default;

void NewTabPageAdEventViewed::Trigger(
    const NewTabPageAdInfo& ad) {
  const auto permission_rules = CreatePermissionRules();
  if (!ads_->IsAdAllowed(permission_rules)) {
    BLOG(1, "New tab page ad: Not allowed based on history");
    return;
  }

  NewTabPageAdWallpaperFrequencyCap new_tab_page_wallpaper_frequency_cap(ads_);
  if (new_tab_page_wallpaper_frequency_cap.ShouldExclude(ad)) {
    if (!new_tab_page_wallpaper_frequency_cap.get_last_message().empty()) {
      BLOG(2, new_tab_page_wallpaper_frequency_cap.get_last_message());
    }

    BLOG(1, "New tab page ad: Not allowed based on history");
    return;
  }

  BLOG(3, "Viewed new tab page ad with uuid " << ad.uuid
      << " and creative instance id " << ad.creative_instance_id);

  ads_->get_client()->AppendUuidToNewTabPageAdHistory(ad.uuid);

  ads_->AppendNewTabPageAdToHistory(ad, kConfirmationType);

  ads_->get_confirmations()->ConfirmAd(ad.creative_instance_id,
      kConfirmationType);
}

std::vector<std::unique_ptr<PermissionRule>>
NewTabPageAdEventViewed::CreatePermissionRules() const {
  std::vector<std::unique_ptr<PermissionRule>> permission_rules;

  std::unique_ptr<PermissionRule> ads_per_hour_frequency_cap =
      std::make_unique<NewTabPageAdsPerHourFrequencyCap>(ads_);
  permission_rules.push_back(std::move(ads_per_hour_frequency_cap));

  std::unique_ptr<PermissionRule> ads_per_day_frequency_cap =
      std::make_unique<NewTabPageAdsPerDayFrequencyCap>(ads_);
  permission_rules.push_back(std::move(ads_per_day_frequency_cap));

  return permission_rules;
}

}  // namespace ads
