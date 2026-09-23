/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_URL_PATTERN_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_URL_PATTERN_CONSTANTS_H_

#include <string>

namespace brave_ads {

const std::string& GetAmazonUrlPattern();
const std::string& GetGoogleUrlPattern();
const std::string& GetMojeekUrlPattern();
const std::string& GetWikipediaUrlPattern();
const std::string& GetYahooUrlPattern();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_URL_PATTERN_CONSTANTS_H_
