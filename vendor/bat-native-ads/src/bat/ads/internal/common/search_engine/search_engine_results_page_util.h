/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_RESULTS_PAGE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_RESULTS_PAGE_UTIL_H_

#include <string>

#include "absl/types/optional.h"

class GURL;

namespace ads {

bool IsSearchEngineResultsPage(const GURL& url);

absl::optional<std::string> ExtractSearchTermQueryValue(const GURL& url);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_RESULTS_PAGE_UTIL_H_
