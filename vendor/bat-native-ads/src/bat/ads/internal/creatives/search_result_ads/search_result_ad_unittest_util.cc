/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/search_result_ads/search_result_ad_unittest_util.h"

#include "base/guid.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "url/gurl.h"

namespace ads {

mojom::SearchResultAdInfoPtr BuildSearchResultAd() {
  mojom::SearchResultAdInfoPtr ad = mojom::SearchResultAdInfo::New();

  ad->creative_instance_id = base::GUID::GenerateRandomV4().AsLowercaseString();
  ad->placement_id = base::GUID::GenerateRandomV4().AsLowercaseString();
  ad->creative_set_id = base::GUID::GenerateRandomV4().AsLowercaseString();
  ad->campaign_id = base::GUID::GenerateRandomV4().AsLowercaseString();
  ad->advertiser_id = base::GUID::GenerateRandomV4().AsLowercaseString();
  ad->target_url = GURL("https://brave.com");
  ad->headline_text = "headline";
  ad->description = "description";
  ad->value = 1.0;
  ad->conversion = mojom::ConversionInfo::New();
  ad->conversion->type = "postview";
  ad->conversion->url_pattern = "https://brave.com/*";
  ad->conversion->advertiser_public_key =
      "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  ad->conversion->observation_window = 3;
  ad->conversion->expire_at = DistantFuture();

  return ad;
}

}  // namespace ads
