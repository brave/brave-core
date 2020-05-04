/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UNITTEST_UTILS_H_
#define BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UNITTEST_UTILS_H_

#include <stdint.h>

#include <string>

namespace ads {

class Client;

void GeneratePastCreativeSetHistoryFromNow(
    Client* client,
    const std::string& creative_set_id,
    const int64_t time_offset_in_seconds,
    const uint8_t count);

void GeneratePastCampaignHistoryFromNow(
    Client* client,
    const std::string& campaign_id,
    const int64_t time_offset_in_seconds,
    const uint8_t count);

void GeneratePastAdsHistoryFromNow(
    Client* client,
    const std::string& creative_instance_id,
    const int64_t time_offset_in_seconds,
    const uint8_t count);

void GeneratePastAdConversionHistoryFromNow(
    Client* client,
    const std::string& creative_set_id,
    const int64_t time_offset_in_seconds,
    const uint8_t count);

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UNITTEST_UTILS_H_  // NOLINT
