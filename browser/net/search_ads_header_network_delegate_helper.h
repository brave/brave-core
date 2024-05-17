/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_SEARCH_ADS_HEADER_NETWORK_DELEGATE_HELPER_H_
#define BRAVE_BROWSER_NET_SEARCH_ADS_HEADER_NETWORK_DELEGATE_HELPER_H_

#include <memory>

#include "brave/browser/net/url_context.h"

namespace net {
class HttpRequestHeaders;
}  // namespace net

namespace brave {

inline constexpr char kSearchAdsHeader[] = "Brave-Search-Ads";
inline constexpr char kSearchAdsDisabledValue[] = "?0";

int OnBeforeStartTransaction_SearchAdsHeader(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx);

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_SEARCH_ADS_HEADER_NETWORK_DELEGATE_HELPER_H_
