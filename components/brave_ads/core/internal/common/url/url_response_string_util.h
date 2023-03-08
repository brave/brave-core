/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_RESPONSE_STRING_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_RESPONSE_STRING_UTIL_H_

#include <string>

#include "brave/components/brave_ads/common/interfaces/ads.mojom-forward.h"

namespace ads {

std::string UrlResponseToString(const mojom::UrlResponseInfo& url_response);
std::string UrlResponseHeadersToString(
    const mojom::UrlResponseInfo& url_response);

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_RESPONSE_STRING_UTIL_H_
