/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/inline_content_ad/inline_content_ad_test_util.h"

#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/inline_content_ad_builder.h"
#include "brave/components/brave_ads/core/public/ad_units/inline_content_ad/inline_content_ad_info.h"

namespace brave_ads::test {

InlineContentAdInfo BuildInlineContentAd(
    const bool should_generate_random_uuids) {
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd(should_generate_random_uuids);
  return BuildInlineContentAd(creative_ad);
}

InlineContentAdInfo BuildAndSaveInlineContentAd(
    const bool should_generate_random_uuids) {
  const CreativeInlineContentAdInfo creative_ad =
      BuildCreativeInlineContentAd(should_generate_random_uuids);
  database::SaveCreativeInlineContentAds({creative_ad});
  return BuildInlineContentAd(creative_ad);
}

}  // namespace brave_ads::test
