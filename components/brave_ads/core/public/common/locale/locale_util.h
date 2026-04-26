/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_COMMON_LOCALE_LOCALE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_COMMON_LOCALE_LOCALE_UTIL_H_

#include <string>

#include "brave/components/brave_ads/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_ADS));

namespace brave_ads {

inline constexpr char kDefaultLanguageCode[] = "en";
inline constexpr char kDefaultCountryCode[] = "US";

// Retrieves the current language code. The result of the first call is cached.
// If the language code is changed, the application must be restarted to ensure
// the new value is applied.
std::string CurrentLanguageCode();

// Retrieves the current country code. The result of the first call is cached.
// If the country code is changed, the application must be restarted to ensure
// the new value is applied.
std::string CurrentCountryCode();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_COMMON_LOCALE_LOCALE_UTIL_H_
