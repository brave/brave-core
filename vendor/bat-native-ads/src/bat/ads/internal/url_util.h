/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_URL_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_URL_UTIL_H_

#include <string>
#include <vector>

namespace ads {

bool DoesUrlMatchPattern(const std::string& url, const std::string& pattern);

bool DoesUrlHaveSchemeHTTPOrHTTPS(const std::string& url);

std::string GetHostFromUrl(const std::string& url);

bool SameDomainOrHost(const std::string& lhs, const std::string& rhs);

bool DomainOrHostExists(const std::vector<std::string>& urls,
                        const std::string& url);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_URL_UTIL_H_
