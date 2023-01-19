/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/catalog_campaign_info.h"

#include "bat/ads/internal/common/numbers/number_util.h"

namespace ads {

CatalogCampaignInfo::CatalogCampaignInfo() = default;

CatalogCampaignInfo::CatalogCampaignInfo(const CatalogCampaignInfo& other) =
    default;

CatalogCampaignInfo& CatalogCampaignInfo::operator=(
    const CatalogCampaignInfo& other) = default;

CatalogCampaignInfo::CatalogCampaignInfo(CatalogCampaignInfo&& other) noexcept =
    default;

CatalogCampaignInfo& CatalogCampaignInfo::operator=(
    CatalogCampaignInfo&& other) noexcept = default;

CatalogCampaignInfo::~CatalogCampaignInfo() = default;

bool CatalogCampaignInfo::operator==(const CatalogCampaignInfo& other) const {
  return campaign_id == other.campaign_id && priority == other.priority &&
         DoubleEquals(ptr, other.ptr) && start_at == other.start_at &&
         end_at == other.end_at && daily_cap == other.daily_cap &&
         advertiser_id == other.advertiser_id &&
         creative_sets == other.creative_sets && dayparts == other.dayparts &&
         geo_targets == other.geo_targets;
}

bool CatalogCampaignInfo::operator!=(const CatalogCampaignInfo& other) const {
  return !(*this == other);
}

}  // namespace ads
