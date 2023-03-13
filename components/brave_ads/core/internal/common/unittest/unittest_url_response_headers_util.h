/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_URL_RESPONSE_HEADERS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_URL_RESPONSE_HEADERS_UTIL_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"

namespace brave_ads {

base::flat_map<std::string, std::string> ToUrlResponseHeaders(
    const std::vector<std::string>& headers);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_URL_RESPONSE_HEADERS_UTIL_H_
