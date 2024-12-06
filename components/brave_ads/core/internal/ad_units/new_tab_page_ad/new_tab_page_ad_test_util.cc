/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ad_units/new_tab_page_ad/new_tab_page_ad_test_util.h"

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"

namespace brave_ads::test {

NewTabPageAdInfo BuildNewTabPageAd(bool should_generate_random_uuids) {
  const CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAd(should_generate_random_uuids);
  return BuildNewTabPageAd(creative_ad);
}

NewTabPageAdInfo BuildAndSaveNewTabPageAd(bool should_generate_random_uuids) {
  const CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAd(should_generate_random_uuids);
  database::SaveCreativeNewTabPageAds({creative_ad});
  return BuildNewTabPageAd(creative_ad);
}

}  // namespace brave_ads::test
