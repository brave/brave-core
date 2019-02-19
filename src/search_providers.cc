/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "search_providers.h"

namespace ads {

SearchProviders::SearchProviders() = default;
SearchProviders::~SearchProviders() = default;

bool SearchProviders::IsSearchEngine(const UrlComponents& components) {
  if (components.hostname.empty()) {
    return false;
  }

  auto is_a_search = false;

  for (const auto& search_provider : _search_providers) {
    if (search_provider.hostname.empty()) {
      continue;
    }

    if (search_provider.is_always_classed_as_a_search &&
        components.hostname == search_provider.hostname) {
      is_a_search = true;
      break;
    }

    size_t index = search_provider.search_template.find('{');
    std::string substring = search_provider.search_template.substr(0, index);
    size_t href_index = components.url.find(substring);

    if (index != std::string::npos && href_index != std::string::npos) {
      is_a_search = true;
      break;
    }
  }

  return is_a_search;
}

}  // namespace ads
