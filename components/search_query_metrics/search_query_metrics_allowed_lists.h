// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_ALLOWED_LISTS_H_
#define BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_ALLOWED_LISTS_H_

#include <string_view>

#include "base/containers/fixed_flat_set.h"

namespace metrics {

inline constexpr auto kAllowedCountries =
    base::MakeFixedFlatSet<std::string_view>(
        base::sorted_unique,
        {"BR", "CA", "CO", "DE", "ES", "FR", "GB", "IN", "IT", "JP", "MX", "NL",
         "PH", "PL", "US"});

inline constexpr auto kAllowedLanguages =
    base::MakeFixedFlatSet<std::string_view>(
        base::sorted_unique,
        {"de", "en", "es", "fr", "hi", "it", "ja", "nl", "pl", "pt", "tl"});

inline constexpr auto kAllowedDefaultSearchEngines =
    base::MakeFixedFlatSet<std::string_view>(
        base::sorted_unique,
        {"Bing", "Brave", "DuckDuckGo", "Ecosia", "Google", "Qwant",
         "Startpage", "Yahoo! JAPAN"});

inline constexpr auto kAllowedSearchEngines =
    base::MakeFixedFlatSet<std::string_view>(base::sorted_unique,
                                             {
                                                 "Bing",
                                                 "Brave",
                                                 "ChatGPT",
                                                 "DuckDuckGo",
                                                 "Ecosia",
                                                 "Google",
                                                 "Perplexity",
                                                 "Qwant",
                                                 "Startpage",
                                                 "Yahoo! JAPAN",
                                             });

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_ALLOWED_LISTS_H_
