/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CLIENT_MOCK_H_
#define BAT_ADS_INTERNAL_CLIENT_MOCK_H_

#include <string>

#include "bat/ads/internal/client.h"

namespace ads {

class ClientMock : public Client {
 public:
  ClientMock(
      AdsImpl* ads,
      AdsClient* ads_client)
      : Client(ads, ads_client) {}

  void GeneratePastAdHistoryFromNow(
      const std::string& creative_instance_id,
      const int64_t time_offset_per_ad_in_seconds,
      const uint8_t count);

  void GeneratePastCreativeSetHistoryFromNow(
      const std::string& creative_set_id,
      const int64_t time_offset_per_ad_in_seconds,
      const uint8_t count);

    void GeneratePastCampaignHistoryFromNow(
      const std::string& campaign_id,
      const int64_t time_offset_per_ad_in_seconds,
      const uint8_t count);

  void GeneratePastPurchaseIntentSignalHistoryFromNow(
      const std::string& segment,
      const uint64_t time_offset_in_seconds,
      const uint16_t weight);

  void GeneratePastAdConversionHistoryFromNow(
      const std::string& creative_set_id,
      const int64_t time_offset_per_ad_in_seconds,
      const uint8_t count);
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CLIENT_MOCK_H_
