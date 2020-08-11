/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_LOGGING_UTIL_H_
#define BAT_ADS_INTERNAL_LOGGING_UTIL_H_

#include <string>

#include "bat/ads/mojom.h"

namespace ads {

std::string UrlRequestToString(
    const UrlRequestPtr& url_request);

std::string UrlRequestHeadersToString(
    const UrlRequestPtr& url_request);

std::string UrlResponseToString(
    const UrlResponse& url_response);

std::string UrlResponseHeadersToString(
    const UrlResponse& url_response);

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_LOGGING_UTIL_H_
