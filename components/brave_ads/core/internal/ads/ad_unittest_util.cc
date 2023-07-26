/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"

#include "base/uuid.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/ad_info.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "url/gurl.h"

namespace brave_ads {

void DisableBraveRewards() {
  SetDefaultBooleanPref(brave_rewards::prefs::kEnabled, false);
}

void DisableBraveNewsAds() {
  SetDefaultBooleanPref(brave_news::prefs::kBraveNewsOptedIn, false);
  SetDefaultBooleanPref(brave_news::prefs::kNewTabPageShowToday, false);
}

void DisableNewTabPageAds() {
  SetDefaultBooleanPref(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, false);
  SetDefaultBooleanPref(ntp_background_images::prefs::
                            kNewTabPageShowSponsoredImagesBackgroundImage,
                        false);
}

void DisableNotificationAds() {
  SetDefaultBooleanPref(prefs::kOptedInToNotificationAds, false);
}

AdInfo BuildAd(const AdType& ad_type, const bool should_use_random_uuids) {
  AdInfo ad;

  ad.type = ad_type;

  ad.placement_id = should_use_random_uuids
                        ? base::Uuid::GenerateRandomV4().AsLowercaseString()
                        : kPlacementId;

  ad.creative_instance_id =
      should_use_random_uuids
          ? base::Uuid::GenerateRandomV4().AsLowercaseString()
          : kCreativeInstanceId;

  ad.creative_set_id = should_use_random_uuids
                           ? base::Uuid::GenerateRandomV4().AsLowercaseString()
                           : kCreativeSetId;

  ad.campaign_id = should_use_random_uuids
                       ? base::Uuid::GenerateRandomV4().AsLowercaseString()
                       : kCampaignId;

  ad.advertiser_id = should_use_random_uuids
                         ? base::Uuid::GenerateRandomV4().AsLowercaseString()
                         : kAdvertiserId;

  ad.segment = kSegment;

  ad.target_url = GURL("https://brave.com");

  return ad;
}

}  // namespace brave_ads
