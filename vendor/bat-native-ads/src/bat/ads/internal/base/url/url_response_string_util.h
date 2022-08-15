/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_URL_URL_RESPONSE_STRING_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_URL_URL_RESPONSE_STRING_UTIL_H_

#include <string>

#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

std::string UrlResponseToString(const mojom::UrlResponseInfo& url_response);
std::string UrlResponseHeadersToString(
    const mojom::UrlResponseInfo& url_response);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_URL_URL_RESPONSE_STRING_UTIL_H_
