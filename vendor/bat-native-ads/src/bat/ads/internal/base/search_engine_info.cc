/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/search_engine_info.h"

namespace ads {

SearchEngineInfo::SearchEngineInfo() = default;

SearchEngineInfo::SearchEngineInfo(const std::string& name,
                                   const std::string& hostname,
                                   const std::string& query,
                                   bool is_always_classed_as_a_search)
    : name(name),
      hostname(hostname),
      query(query),
      is_always_classed_as_a_search(is_always_classed_as_a_search) {}

SearchEngineInfo::SearchEngineInfo(const SearchEngineInfo& info) = default;

SearchEngineInfo::~SearchEngineInfo() = default;

}  // namespace ads
