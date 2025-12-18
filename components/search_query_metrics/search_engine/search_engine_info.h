/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_ENGINE_SEARCH_ENGINE_INFO_H_
#define BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_ENGINE_SEARCH_ENGINE_INFO_H_

#include <string_view>

namespace metrics {

struct SearchEngineInfo final {
  constexpr SearchEngineInfo(std::string_view name,
                             std::string_view results_page_url_pattern,
                             std::string_view search_term_query_key)
      : name(name),
        results_page_url_pattern(results_page_url_pattern),
        search_term_query_key(search_term_query_key) {}

  std::string_view name;
  std::string_view results_page_url_pattern;
  std::string_view search_term_query_key;
};

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_ENGINE_SEARCH_ENGINE_INFO_H_
