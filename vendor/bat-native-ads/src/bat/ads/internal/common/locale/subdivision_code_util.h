/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_LOCALE_SUBDIVISION_CODE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_LOCALE_SUBDIVISION_CODE_UTIL_H_

#include <string>

namespace ads::locale {

bool IsSupportedCountryCodeForSubdivisionTargeting(
    const std::string& country_code);

std::string GetCountryCode(const std::string& code);

std::string GetSubdivisionCode(const std::string& code);

}  // namespace ads::locale

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_LOCALE_SUBDIVISION_CODE_UTIL_H_
