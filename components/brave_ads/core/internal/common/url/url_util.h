/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_UTIL_H_

#include <string>
#include <vector>

class GURL;

namespace brave_ads {

GURL GetUrlWithEmptyQuery(const GURL& url);

bool SchemeIsSupportedForUrl(const GURL& url);

bool MatchUrlPattern(const GURL& url, const std::string& pattern);

bool SameHostForUrl(const GURL& lhs, const GURL& rhs);
bool HostForUrlExists(const std::vector<GURL>& lhs, const GURL& rhs);

bool SameDomainOrHostForUrl(const GURL& lhs, const GURL& rhs);
bool DomainOrHostForUrlExists(const std::vector<GURL>& lhs, const GURL& rhs);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_UTIL_H_
