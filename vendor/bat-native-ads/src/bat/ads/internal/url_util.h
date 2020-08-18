/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_URL_UTIL_H_
#define BAT_ADS_INTERNAL_URL_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ads/ads_client.h"

namespace ads {

bool UrlMatchesPattern(
    const std::string& url,
    const std::string& pattern);

bool UrlHasScheme(
    const std::string& url);

bool SameSite(
    const std::string& url1,
    const std::string& url2);

std::string GetUrlMethodName(
    const URLRequestMethod method);

std::map<std::string, std::string> NormalizeHeaders(
    const std::vector<std::string>& headers);

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_URL_UTIL_H_
