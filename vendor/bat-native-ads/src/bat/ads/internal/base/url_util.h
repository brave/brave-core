/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_URL_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_URL_UTIL_H_

#include <string>
#include <vector>

class GURL;

namespace ads {

bool DoesUrlMatchPattern(const GURL& url, const std::string& pattern);

bool SameDomainOrHost(const GURL& lhs, const GURL& rhs);
bool DomainOrHostExists(const std::vector<GURL>& urls, const GURL& url);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_URL_UTIL_H_
