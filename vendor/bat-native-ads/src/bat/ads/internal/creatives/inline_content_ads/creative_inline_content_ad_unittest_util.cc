/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_unittest_util.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "bat/ads/internal/creatives/creative_ad_unittest_util.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"
#include "bat/ads/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"
#include "url/gurl.h"

namespace ads {

void SaveCreativeAds(const CreativeInlineContentAdList& creative_ads) {
  database::table::CreativeInlineContentAds database_table;
  database_table.Save(
      creative_ads, base::BindOnce([](const bool success) { CHECK(success); }));
}

CreativeInlineContentAdList BuildCreativeInlineContentAds(const int count) {
  CreativeInlineContentAdList creative_ads;

  for (int i = 0; i < count; i++) {
    const CreativeInlineContentAdInfo creative_ad =
        BuildCreativeInlineContentAd(/*should_use_random_guids*/ true);
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

CreativeInlineContentAdInfo BuildCreativeInlineContentAd(
    const bool should_use_random_guids) {
  const CreativeAdInfo creative_ad = BuildCreativeAd(should_use_random_guids);
  CreativeInlineContentAdInfo creative_inline_content_ad(creative_ad);

  creative_inline_content_ad.title = "Test Ad Title";
  creative_inline_content_ad.description = "Test Ad Description";
  creative_inline_content_ad.image_url = GURL("https://brave.com/image");
  creative_inline_content_ad.dimensions = "200x100";
  creative_inline_content_ad.cta_text = "Call to action text";

  return creative_inline_content_ad;
}

}  // namespace ads
