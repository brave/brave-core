/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_SUPPORTED_COUNTRY_CODES_ALIASES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_SUPPORTED_COUNTRY_CODES_ALIASES_H_

#include <map>
#include <set>
#include <string>

namespace ads {

using SupportedCountryCodesSet = std::set<std::string>;
using SupportedCountryCodesMap = std::map<int, SupportedCountryCodesSet>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_LOCALE_SUPPORTED_COUNTRY_CODES_ALIASES_H_
