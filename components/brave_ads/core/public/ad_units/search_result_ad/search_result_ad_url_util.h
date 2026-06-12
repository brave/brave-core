/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_UNITS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_URL_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_UNITS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_URL_UTIL_H_

#include "brave/components/brave_ads/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_ADS));

class GURL;

namespace brave_ads {

// Returns `true` if `url` is a search result ad redirect URL.
bool IsSearchResultAdRedirectUrl(const GURL& url);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_AD_UNITS_SEARCH_RESULT_AD_SEARCH_RESULT_AD_URL_UTIL_H_
