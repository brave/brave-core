/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_URL_URL_REQUEST_STRING_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_URL_URL_REQUEST_STRING_UTIL_H_

#include <string>

#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace ads {

std::string UrlRequestToString(const mojom::UrlRequestInfoPtr& url_request);
std::string UrlRequestHeadersToString(
    const mojom::UrlRequestInfoPtr& url_request);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_URL_URL_REQUEST_STRING_UTIL_H_
