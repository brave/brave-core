/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_UTIL_H_

class GURL;

namespace brave_ads {

[[nodiscard]] bool IsSearchEngine(const GURL& url);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_UTIL_H_
