/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/test/campaign_conversion_test_util.h"

#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_database_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"

namespace brave_ads::test {

CampaignConversionInfo BuildCampaignConversion(
    const std::string& campaign_id,
    const std::string& url_pattern,
    base::TimeDelta observation_window) {
  CampaignConversionInfo campaign_conversion;

  campaign_conversion.id = campaign_id;
  campaign_conversion.url_pattern = url_pattern;
  campaign_conversion.observation_window = observation_window;
  campaign_conversion.expire_at =
      Now() + campaign_conversion.observation_window;

  return campaign_conversion;
}

void BuildAndSaveCampaignConversion(const std::string& campaign_id,
                                    const std::string& url_pattern,
                                    base::TimeDelta observation_window) {
  CampaignConversionList campaign_conversions;

  campaign_conversions.push_back(
      BuildCampaignConversion(campaign_id, url_pattern, observation_window));

  database::SaveCampaignConversions(campaign_conversions);
}

}  // namespace brave_ads::test
