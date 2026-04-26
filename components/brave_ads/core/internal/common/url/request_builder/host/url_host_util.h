/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_REQUEST_BUILDER_HOST_URL_HOST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_REQUEST_BUILDER_HOST_URL_HOST_UTIL_H_

#include <string>

#include "brave/components/brave_ads/core/internal/common/url/request_builder/host/url_host_types.h"

namespace brave_ads {

// Returns the URL host for the given `type`. See `UrlHostType` for
// usage guidance on each value.
std::string GetUrlHost(UrlHostType type);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_URL_REQUEST_BUILDER_HOST_URL_HOST_UTIL_H_
