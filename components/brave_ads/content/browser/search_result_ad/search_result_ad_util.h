/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_UTIL_H_

#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"

class GURL;

namespace brave_ads {

absl::optional<std::string> GetPlacementIdFromSearchResultAdClickedUrl(
    const GURL& url);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CONTENT_BROWSER_SEARCH_RESULT_AD_SEARCH_RESULT_AD_UTIL_H_
