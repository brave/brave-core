/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_INLINE_CONTENT_ADS_CREATIVE_INLINE_CONTENT_AD_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_INLINE_CONTENT_ADS_CREATIVE_INLINE_CONTENT_AD_UNITTEST_UTIL_H_

#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ad_info.h"

namespace brave_ads::test {

CreativeInlineContentAdList BuildCreativeInlineContentAds(int count);
CreativeInlineContentAdInfo BuildCreativeInlineContentAd(
    bool should_use_random_uuids);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_INLINE_CONTENT_ADS_CREATIVE_INLINE_CONTENT_AD_UNITTEST_UTIL_H_
