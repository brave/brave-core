/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_URL_PATTERN_CONSTANTS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_URL_PATTERN_CONSTANTS_H_

#include <string>

namespace ads {

const std::string& GetAmazonUrlPattern();
const std::string& GetGoogleUrlPattern();
const std::string& GetMojeekUrlPattern();
const std::string& GetWikipediaUrlPattern();
const std::string& GetYahooUrlPattern();

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_URL_PATTERN_CONSTANTS_H_
