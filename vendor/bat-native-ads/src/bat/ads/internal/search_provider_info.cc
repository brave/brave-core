/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/search_provider_info.h"

namespace ads {

SearchProviderInfo::SearchProviderInfo() = default;

SearchProviderInfo::SearchProviderInfo(
    const std::string& name,
    const std::string& hostname,
    const std::string& search_template,
    bool is_always_classed_as_a_search)
    : name(name),
      hostname(hostname),
      search_template(search_template),
      is_always_classed_as_a_search(is_always_classed_as_a_search) {}

SearchProviderInfo::SearchProviderInfo(
    const SearchProviderInfo& info) = default;

SearchProviderInfo::~SearchProviderInfo() = default;

}  // namespace ads
