// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_BRAVE_SEARCH_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_BRAVE_SEARCH_UTILS_H_

#include <string_view>

#include "base/containers/fixed_flat_set.h"

class GURL;
class PrefService;

namespace brave_search {

inline constexpr auto kVettedHosts = base::MakeFixedFlatSet<std::string_view>(
    base::sorted_unique,
    {
        "https://safesearch.brave.com",
        "https://safesearch.brave.software",
        "https://safesearch.bravesoftware.com",
        "https://search-dev-local.brave.com",
        "https://search.brave.com",
        "https://search.brave.software",
        "https://search.bravesoftware.com",
    });

bool IsAllowedHost(const GURL& url);
bool IsDefaultAPIEnabled();

// Appends the `source` query param (`newtab`, `newtab_v1` or `newtab_v2`) to
// `url` so Brave Search can distinguish NTP searchbox traffic from other
// entry points. Should only be called when `url` is a Brave Search URL.
GURL OverrideWithNewTabSource(GURL url,
                              PrefService* local_state,
                              bool is_first_run);

}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_BRAVE_SEARCH_UTILS_H_
