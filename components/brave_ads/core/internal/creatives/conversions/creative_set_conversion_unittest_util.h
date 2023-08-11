/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_UNITTEST_UTIL_H_

#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

struct CreativeSetConversionInfo;

CreativeSetConversionInfo BuildVerifiableCreativeSetConversionForTesting(
    const std::string& creative_set_id,
    const std::string& url_pattern,
    base::TimeDelta observation_window,
    const absl::optional<std::string>& verifiable_advertiser_public_key_base64);
void BuildAndSaveVerifiableCreativeSetConversionForTesting(
    const std::string& creative_set_id,
    const std::string& url_pattern,
    base::TimeDelta observation_window,
    const absl::optional<std::string>& verifiable_advertiser_public_key_base64);

CreativeSetConversionInfo BuildCreativeSetConversionForTesting(
    const std::string& creative_set_id,
    const std::string& url_pattern,
    base::TimeDelta observation_window);
void BuildAndSaveCreativeSetConversionForTesting(
    const std::string& creative_set_id,
    const std::string& url_pattern,
    base::TimeDelta observation_window);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_CREATIVE_SET_CONVERSION_UNITTEST_UTIL_H_
