/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/creative_search_result_ad_test_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_test_constants.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "url/gurl.h"

namespace brave_ads::test {

namespace {

constexpr char kConversionUrlPattern[] = "https://brave.com/*";
constexpr base::TimeDelta kConversionObservationWindow = base::Days(3);

}  // namespace

mojom::CreativeSearchResultAdInfoPtr BuildCreativeSearchResultAd(
    bool should_generate_random_uuids) {
  mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      mojom::CreativeSearchResultAdInfo::New();

  mojom_creative_ad->placement_id =
      RandomUuidOr(should_generate_random_uuids, kPlacementId);
  mojom_creative_ad->creative_instance_id =
      RandomUuidOr(should_generate_random_uuids, kCreativeInstanceId);
  mojom_creative_ad->creative_set_id =
      RandomUuidOr(should_generate_random_uuids, kCreativeSetId);
  mojom_creative_ad->campaign_id =
      RandomUuidOr(should_generate_random_uuids, kCampaignId);
  mojom_creative_ad->advertiser_id =
      RandomUuidOr(should_generate_random_uuids, kAdvertiserId);
  mojom_creative_ad->target_url = GURL(kTargetUrl);
  mojom_creative_ad->headline_text = kTitle;
  mojom_creative_ad->description = kDescription;
  mojom_creative_ad->value = kValue;

  return mojom_creative_ad;
}

mojom::CreativeSearchResultAdInfoPtr BuildCreativeSearchResultAdWithConversion(
    bool should_generate_random_uuids) {
  mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      BuildCreativeSearchResultAd(should_generate_random_uuids);
  CHECK(mojom_creative_ad);

  mojom_creative_ad->creative_set_conversion =
      mojom::CreativeSetConversionInfo::New();
  mojom_creative_ad->creative_set_conversion->url_pattern =
      kConversionUrlPattern;
  mojom_creative_ad->creative_set_conversion
      ->verifiable_advertiser_public_key_base64 =
      kVerifiableConversionAdvertiserPublicKeyBase64;
  mojom_creative_ad->creative_set_conversion->observation_window =
      kConversionObservationWindow;

  return mojom_creative_ad;
}

}  // namespace brave_ads::test
