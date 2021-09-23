/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_campaign_info.h"

#include "bat/ads/internal/number_util.h"

namespace ads {

CatalogCampaignInfo::CatalogCampaignInfo() = default;

CatalogCampaignInfo::CatalogCampaignInfo(const CatalogCampaignInfo& info) =
    default;

CatalogCampaignInfo::~CatalogCampaignInfo() = default;

bool CatalogCampaignInfo::operator==(const CatalogCampaignInfo& rhs) const {
  return campaign_id == rhs.campaign_id && priority == rhs.priority &&
         DoubleEquals(ptr, rhs.ptr) && start_at == rhs.start_at &&
         end_at == rhs.end_at && daily_cap == rhs.daily_cap &&
         advertiser_id == rhs.advertiser_id &&
         creative_sets == rhs.creative_sets && dayparts == rhs.dayparts &&
         geo_targets == rhs.geo_targets;
}

bool CatalogCampaignInfo::operator!=(const CatalogCampaignInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
