/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_COMMON_SEARCH_RESULT_AD_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_COMMON_SEARCH_RESULT_AD_UTIL_H_

#include <string>

class GURL;

namespace network {
struct ResourceRequest;
}  // namespace network

namespace brave_ads {

bool IsSearchResultAdViewedConfirmationUrl(const GURL& url);

std::string GetViewedSearchResultAdCreativeInstanceId(
    const network::ResourceRequest& request);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_COMMON_SEARCH_RESULT_AD_UTIL_H_
