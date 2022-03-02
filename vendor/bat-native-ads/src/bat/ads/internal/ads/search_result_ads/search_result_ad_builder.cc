/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/search_result_ads/search_result_ad_builder.h"

#include "base/guid.h"
#include "bat/ads/internal/bundle/creative_search_result_ad_info.h"
#include "bat/ads/search_result_ad_info.h"

namespace ads {

SearchResultAdInfo BuildSearchResultAd(
    const CreativeSearchResultAdInfo& creative_search_result_ad) {
  const std::string uuid = base::GenerateGUID();
  return BuildSearchResultAd(creative_search_result_ad, uuid);
}

SearchResultAdInfo BuildSearchResultAd(
    const CreativeSearchResultAdInfo& creative_search_result_ad,
    const std::string& uuid) {
  SearchResultAdInfo search_result_ad;

  search_result_ad.type = AdType::kSearchResultAd;
  search_result_ad.uuid = uuid;
  search_result_ad.creative_instance_id =
      creative_search_result_ad.creative_instance_id;
  search_result_ad.creative_set_id = creative_search_result_ad.creative_set_id;
  search_result_ad.campaign_id = creative_search_result_ad.campaign_id;
  search_result_ad.advertiser_id = creative_search_result_ad.advertiser_id;
  search_result_ad.segment = creative_search_result_ad.segment;
  search_result_ad.target_url = creative_search_result_ad.target_url;
  search_result_ad.title = creative_search_result_ad.title;
  search_result_ad.body = creative_search_result_ad.body;

  return search_result_ad;
}

}  // namespace ads
