/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad_builder.h"

#include "base/guid.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_info.h"
#include "bat/ads/new_tab_page_ad_info.h"

namespace ads {

NewTabPageAdInfo BuildNewTabPageAd(
    const CreativeNewTabPageAdInfo& creative_new_tab_page_ad) {
  const std::string uuid = base::GenerateGUID();
  return BuildNewTabPageAd(creative_new_tab_page_ad, uuid);
}

NewTabPageAdInfo BuildNewTabPageAd(
    const CreativeNewTabPageAdInfo& creative_new_tab_page_ad,
    const std::string& uuid) {
  NewTabPageAdInfo new_tab_page_ad;

  new_tab_page_ad.type = AdType::kNewTabPageAd;
  new_tab_page_ad.uuid = uuid;
  new_tab_page_ad.creative_instance_id =
      creative_new_tab_page_ad.creative_instance_id;
  new_tab_page_ad.creative_set_id = creative_new_tab_page_ad.creative_set_id;
  new_tab_page_ad.campaign_id = creative_new_tab_page_ad.campaign_id;
  new_tab_page_ad.advertiser_id = creative_new_tab_page_ad.advertiser_id;
  new_tab_page_ad.segment = creative_new_tab_page_ad.segment;
  new_tab_page_ad.target_url = creative_new_tab_page_ad.target_url;
  new_tab_page_ad.company_name = creative_new_tab_page_ad.company_name;
  new_tab_page_ad.alt = creative_new_tab_page_ad.alt;

  return new_tab_page_ad;
}

}  // namespace ads
