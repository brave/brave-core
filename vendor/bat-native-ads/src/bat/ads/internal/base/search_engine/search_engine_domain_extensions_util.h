/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_SEARCH_ENGINE_SEARCH_ENGINE_DOMAIN_EXTENSIONS_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_SEARCH_ENGINE_SEARCH_ENGINE_DOMAIN_EXTENSIONS_UTIL_H_

#include <string>
#include <vector>

namespace ads {

const std::vector<std::string>& GetAmazonSearchEngineDomainExtensions();
const std::vector<std::string>& GetGoogleSearchEngineDomainExtensions();
const std::vector<std::string>& GetMojeekSearchEngineDomainExtensions();

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_SEARCH_ENGINE_SEARCH_ENGINE_DOMAIN_EXTENSIONS_UTIL_H_
