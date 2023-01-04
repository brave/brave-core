/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_INFO_H_

#include <string>

namespace ads {

struct SearchEngineInfo final {
  SearchEngineInfo(std::string url_pattern,
                   std::string result_page_url_pattern,
                   std::string search_term_query_key);

  std::string url_pattern;
  std::string result_page_url_pattern;
  std::string search_term_query_key;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_SEARCH_ENGINE_SEARCH_ENGINE_INFO_H_
