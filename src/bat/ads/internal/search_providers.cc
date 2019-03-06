/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/search_providers.h"

#include "url/gurl.h"

namespace ads {

SearchProviders::SearchProviders() = default;
SearchProviders::~SearchProviders() = default;

bool SearchProviders::IsSearchEngine(const std::string& url) {
  auto gurl = GURL(url);

  if (!gurl.has_host()) {
    return false;
  }

  auto is_a_search = false;

  for (const auto& search_provider : _search_providers) {
    if (search_provider.hostname.empty()) {
      continue;
    }

    if (search_provider.is_always_classed_as_a_search &&
        gurl.DomainIs(search_provider.hostname)) {
      is_a_search = true;
      break;
    }

    size_t index = search_provider.search_template.find('{');
    std::string substring = search_provider.search_template.substr(0, index);
    size_t href_index = url.find(substring);

    if (index != std::string::npos && href_index != std::string::npos) {
      is_a_search = true;
      break;
    }
  }

  return is_a_search;
}

}  // namespace ads
