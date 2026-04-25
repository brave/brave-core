/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_util.h"

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"

namespace brave_ads {

mojom::NewTabPageAdInfoPtr ToMojom(
    base::optional_ref<const NewTabPageAdInfo> ad) {
  if (!ad || !ad->IsValid()) {
    return nullptr;
  }

  mojom::NewTabPageAdInfoPtr new_tab_page_ad = mojom::NewTabPageAdInfo::New();
  new_tab_page_ad->placement_id = ad->placement_id;
  new_tab_page_ad->creative_instance_id = ad->creative_instance_id;
  new_tab_page_ad->creative_set_id = ad->creative_set_id;
  new_tab_page_ad->campaign_id = ad->campaign_id;
  new_tab_page_ad->advertiser_id = ad->advertiser_id;
  new_tab_page_ad->segment = ad->segment;
  new_tab_page_ad->company_name = ad->company_name;
  new_tab_page_ad->alt = ad->alt;
  new_tab_page_ad->target_url = ad->target_url;
  return new_tab_page_ad;
}

std::optional<NewTabPageAdInfo> FromMojom(
    const mojom::NewTabPageAdInfoPtr& new_tab_page_ad) {
  if (!new_tab_page_ad) {
    return std::nullopt;
  }

  NewTabPageAdInfo ad;
  ad.type = mojom::AdType::kNewTabPageAd;
  ad.placement_id = new_tab_page_ad->placement_id;
  ad.creative_instance_id = new_tab_page_ad->creative_instance_id;
  ad.creative_set_id = new_tab_page_ad->creative_set_id;
  ad.campaign_id = new_tab_page_ad->campaign_id;
  ad.advertiser_id = new_tab_page_ad->advertiser_id;
  ad.segment = new_tab_page_ad->segment;
  ad.company_name = new_tab_page_ad->company_name;
  ad.alt = new_tab_page_ad->alt;
  ad.target_url = new_tab_page_ad->target_url;
  return ad;
}

}  // namespace brave_ads
