/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_LOCALE_COUNTRY_CODE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_LOCALE_COUNTRY_CODE_UTIL_H_

#include <string>

namespace ads::privacy::locale {

// Return |true| if the given |country_code| is a member of the anonymity set,
// otherwise returns |false|.
bool IsCountryCodeMemberOfAnonymitySet(const std::string& country_code);

// Return |true| if the given |country_code| should be classified as other
// otherwise returns |false|.
bool ShouldClassifyCountryCodeAsOther(const std::string& country_code);

}  // namespace ads::privacy::locale

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_LOCALE_COUNTRY_CODE_UTIL_H_
