/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_TEST_CREATIVE_SET_CONVERSION_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_TEST_CREATIVE_SET_CONVERSION_TEST_UTIL_H_

#include <string>

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

struct CreativeSetConversionInfo;

namespace test {

CreativeSetConversionInfo BuildCreativeSetConversion(
    const std::string& creative_set_id,
    const std::string& url_pattern,
    base::TimeDelta observation_window);
void BuildAndSaveCreativeSetConversion(const std::string& creative_set_id,
                                       const std::string& url_pattern,
                                       base::TimeDelta observation_window);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_TEST_CREATIVE_SET_CONVERSION_TEST_UTIL_H_
