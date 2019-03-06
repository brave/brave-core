/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/campaign_info.h"

namespace ads {

CampaignInfo::CampaignInfo() :
    campaign_id(""),
    name(""),
    start_at(""),
    end_at(""),
    daily_cap(0),
    budget(0),
    advertiser_id(""),
    geo_targets({}),
    creative_sets({}) {}

CampaignInfo::CampaignInfo(const std::string& campaign_id) :
    campaign_id(campaign_id),
    name(""),
    start_at(""),
    end_at(""),
    daily_cap(0),
    budget(0),
    advertiser_id(""),
    geo_targets({}),
    creative_sets({}) {}

CampaignInfo::CampaignInfo(const CampaignInfo& info) :
    campaign_id(info.campaign_id),
    name(info.name),
    start_at(info.start_at),
    end_at(info.end_at),
    daily_cap(info.daily_cap),
    budget(info.budget),
    advertiser_id(info.advertiser_id),
    geo_targets(info.geo_targets),
    creative_sets(info.creative_sets) {}

CampaignInfo::~CampaignInfo() {}

}  // namespace ads
