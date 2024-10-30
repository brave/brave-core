/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_test_util.h"

#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_builder.h"
#include "brave/components/brave_ads/core/internal/ad_units/search_result_ad/search_result_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/creative_search_result_ad_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::test {

SearchResultAdInfo BuildSearchResultAd(
    const bool should_generate_random_uuids) {
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      BuildCreativeSearchResultAd(should_generate_random_uuids);
  return FromMojomBuildSearchResultAd(mojom_creative_ad);
}

}  // namespace brave_ads::test
