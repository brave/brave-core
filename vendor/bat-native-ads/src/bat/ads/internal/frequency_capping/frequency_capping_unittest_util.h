/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UNITTEST_UTIL_H_
#define BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UNITTEST_UTIL_H_

#include <stdint.h>

#include <memory>
#include <string>

namespace ads {

class AdsImpl;

void GeneratePastCreativeSetHistoryFromNow(
    const std::unique_ptr<AdsImpl>& ads,
    const std::string& creative_set_id,
    const uint64_t time_offset_in_seconds,
    const int count);

void GeneratePastCampaignHistoryFromNow(
    const std::unique_ptr<AdsImpl>& ads,
    const std::string& campaign_id,
    const uint64_t time_offset_in_seconds,
    const int count);

void GeneratePastAdsHistoryFromNow(
    const std::unique_ptr<AdsImpl>& ads,
    const std::string& creative_instance_id,
    const uint64_t time_offset_in_seconds,
    const int count);

void GeneratePastAdConversionHistoryFromNow(
    const std::unique_ptr<AdsImpl>& ads,
    const std::string& creative_set_id,
    const uint64_t time_offset_in_seconds,
    const int count);

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_FREQUENCY_CAPPING_FREQUENCY_CAPPING_UNITTEST_UTIL_H_  // NOLINT
