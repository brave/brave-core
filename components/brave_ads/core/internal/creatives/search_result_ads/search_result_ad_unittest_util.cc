/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_unittest_util.h"

#include "base/check.h"
#include "base/uuid.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_unittest_constants.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr char kHeadlineText[] = "headline";
constexpr char kDescription[] = "description";
constexpr int kValue = 1.0;

constexpr char kConversionType[] = "postview";
constexpr char kConversionUrlPattern[] = "https://brave.com/*";
constexpr int kConversionObservationWindow = 3;

}  // namespace

mojom::SearchResultAdInfoPtr BuildSearchResultAd(
    const bool should_use_random_guids) {
  mojom::SearchResultAdInfoPtr ad = mojom::SearchResultAdInfo::New();

  ad->placement_id = should_use_random_guids
                         ? base::Uuid::GenerateRandomV4().AsLowercaseString()
                         : kPlacementId;

  ad->creative_instance_id =
      should_use_random_guids
          ? base::Uuid::GenerateRandomV4().AsLowercaseString()
          : kCreativeInstanceId;

  ad->creative_set_id = should_use_random_guids
                            ? base::Uuid::GenerateRandomV4().AsLowercaseString()
                            : kCreativeSetId;

  ad->campaign_id = should_use_random_guids
                        ? base::Uuid::GenerateRandomV4().AsLowercaseString()
                        : kCampaignId;

  ad->advertiser_id = should_use_random_guids
                          ? base::Uuid::GenerateRandomV4().AsLowercaseString()
                          : kAdvertiserId;

  ad->target_url = GURL("https://brave.com");

  ad->headline_text = kHeadlineText;

  ad->description = kDescription;

  ad->value = kValue;

  ad->conversion = mojom::ConversionInfo::New();

  return ad;
}

mojom::SearchResultAdInfoPtr BuildSearchResultAdWithConversion(
    const bool should_use_random_guids) {
  mojom::SearchResultAdInfoPtr ad =
      BuildSearchResultAd(should_use_random_guids);
  CHECK(ad);
  CHECK(ad->conversion);

  ad->conversion->type = kConversionType;
  ad->conversion->url_pattern = kConversionUrlPattern;
  ad->conversion->advertiser_public_key = kConversionAdvertiserPublicKey;
  ad->conversion->observation_window = kConversionObservationWindow;

  return ad;
}

}  // namespace brave_ads
