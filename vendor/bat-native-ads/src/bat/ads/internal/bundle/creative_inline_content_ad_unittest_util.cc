/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_inline_content_ad_unittest_util.h"

#include "bat/ads/internal/bundle/creative_ad_unittest_util.h"
#include "bat/ads/internal/bundle/creative_inline_content_ad_info.h"

namespace ads {

CreativeInlineContentAdInfo GetCreativeInlineContentAd() {
  const CreativeAdInfo creative_ad = GetCreativeAd();
  CreativeInlineContentAdInfo creative_inline_content_ad(creative_ad);

  creative_inline_content_ad.title = "Test Ad Title";
  creative_inline_content_ad.description = "Test Ad Description";
  creative_inline_content_ad.image_url = "https://brave.com/image";
  creative_inline_content_ad.dimensions = "200x100";
  creative_inline_content_ad.cta_text = "Call to action text";

  return creative_inline_content_ad;
}

CreativeInlineContentAdInfo GetCreativeInlineContentAdForSegment(
    const std::string& segment) {
  CreativeInlineContentAdInfo creative_ad = GetCreativeInlineContentAd();
  creative_ad.segment = segment;
  return creative_ad;
}

}  // namespace ads
