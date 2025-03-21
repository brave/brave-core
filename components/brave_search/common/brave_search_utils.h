// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_BRAVE_SEARCH_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_BRAVE_SEARCH_UTILS_H_

#include <string_view>

#include "base/containers/fixed_flat_set.h"

class GURL;

namespace brave_search {

inline constexpr auto kVettedHosts =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {
                                                 "safesearch.brave.com",
                                                 "safesearch.brave.software",
                                                 "safesearch.bravesoftware.com",
                                                 "search-dev-local.brave.com",
                                                 "search.brave.com",
                                                 "search.brave.software",
                                                 "search.bravesoftware.com",
                                             });

bool IsAllowedHost(const GURL& url);
bool IsDefaultAPIEnabled();

}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_BRAVE_SEARCH_UTILS_H_
