/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/search_engine/search_engine_info.h"

namespace ads {

SearchEngineInfo::SearchEngineInfo(const std::string& url_pattern,
                                   const std::string& result_page_url_pattern,
                                   const std::string& search_term_query_key)
    : url_pattern(url_pattern),
      result_page_url_pattern(result_page_url_pattern),
      search_term_query_key(search_term_query_key) {}

}  // namespace ads
