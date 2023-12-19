/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_unittest_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads::test {

namespace {

constexpr char kHeadlineText[] = "headline";
constexpr char kDescription[] = "description";
constexpr int kValue = 1.0;

constexpr char kConversionUrlPattern[] = "https://brave.com/*";
constexpr base::TimeDelta kConversionObservationWindow = base::Days(3);

}  // namespace

mojom::SearchResultAdInfoPtr BuildSearchResultAd(
    const bool should_use_random_uuids) {
  mojom::SearchResultAdInfoPtr ad = mojom::SearchResultAdInfo::New();

  ad->placement_id = GetConstantId(should_use_random_uuids, kPlacementId);
  ad->creative_instance_id =
      GetConstantId(should_use_random_uuids, kCreativeInstanceId);
  ad->creative_set_id = GetConstantId(should_use_random_uuids, kCreativeSetId);
  ad->campaign_id = GetConstantId(should_use_random_uuids, kCampaignId);
  ad->advertiser_id = GetConstantId(should_use_random_uuids, kAdvertiserId);
  ad->target_url = GURL("https://brave.com");
  ad->headline_text = kHeadlineText;
  ad->description = kDescription;
  ad->value = kValue;

  return ad;
}

mojom::SearchResultAdInfoPtr BuildSearchResultAdWithConversion(
    const bool should_use_random_uuids) {
  mojom::SearchResultAdInfoPtr ad =
      BuildSearchResultAd(should_use_random_uuids);
  CHECK(ad);

  ad->conversion = mojom::ConversionInfo::New();
  ad->conversion->url_pattern = kConversionUrlPattern;
  ad->conversion->verifiable_advertiser_public_key_base64 =
      kVerifiableConversionAdvertiserPublicKey;
  ad->conversion->observation_window = kConversionObservationWindow;

  return ad;
}

}  // namespace brave_ads::test
