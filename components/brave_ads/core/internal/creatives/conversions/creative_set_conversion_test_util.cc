/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_test_util.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table_util.h"

namespace brave_ads::test {

CreativeSetConversionInfo BuildCreativeSetConversion(
    const std::string& creative_set_id,
    const std::string& url_pattern,
    base::TimeDelta observation_window) {
  return BuildVerifiableCreativeSetConversion(
      creative_set_id, url_pattern, observation_window,
      /*verifiable_advertiser_public_key_base64=*/std::nullopt);
}

void BuildAndSaveCreativeSetConversion(const std::string& creative_set_id,
                                       const std::string& url_pattern,
                                       base::TimeDelta observation_window) {
  BuildAndSaveVerifiableCreativeSetConversion(
      creative_set_id, url_pattern, observation_window,
      /*verifiable_advertiser_public_key_base64=*/std::nullopt);
}

CreativeSetConversionInfo BuildVerifiableCreativeSetConversion(
    const std::string& creative_set_id,
    const std::string& url_pattern,
    base::TimeDelta observation_window,
    std::optional<std::string> verifiable_advertiser_public_key_base64) {
  CreativeSetConversionInfo creative_set_conversion;

  creative_set_conversion.id = creative_set_id;
  creative_set_conversion.url_pattern = url_pattern;
  creative_set_conversion.verifiable_advertiser_public_key_base64 =
      std::move(verifiable_advertiser_public_key_base64);
  creative_set_conversion.observation_window = observation_window;
  creative_set_conversion.expire_at =
      Now() + creative_set_conversion.observation_window;

  return creative_set_conversion;
}

void BuildAndSaveVerifiableCreativeSetConversion(
    const std::string& creative_set_id,
    const std::string& url_pattern,
    base::TimeDelta observation_window,
    std::optional<std::string> verifiable_advertiser_public_key_base64) {
  CreativeSetConversionList creative_set_conversions;

  const CreativeSetConversionInfo creative_set_conversion =
      BuildVerifiableCreativeSetConversion(
          creative_set_id, url_pattern, observation_window,
          std::move(verifiable_advertiser_public_key_base64));
  creative_set_conversions.push_back(creative_set_conversion);

  database::SaveCreativeSetConversions(creative_set_conversions);
}

}  // namespace brave_ads::test
