/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_REQUEST_STRING_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_REQUEST_STRING_UTIL_H_

#include <string>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

std::string UrlRequestToString(
    const mojom::UrlRequestInfoPtr& mojom_url_request);
std::string UrlRequestHeadersToString(
    const mojom::UrlRequestInfoPtr& mojom_url_request);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_URL_REQUEST_STRING_UTIL_H_
