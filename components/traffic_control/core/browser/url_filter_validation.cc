// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/url_filter_validation.h"

#include <string>

#include "components/url_matcher/url_util.h"

namespace traffic_control {

bool IsValidUrlFilter(std::string_view filter) {
  if (filter.empty()) {
    return false;
  }
  std::string scheme;
  std::string host;
  bool match_subdomains = false;
  uint16_t port = 0;
  std::string path;
  std::string query;
  return url_matcher::util::FilterToComponents(std::string(filter), &scheme,
                                               &host, &match_subdomains, &port,
                                               &path, &query);
}

}  // namespace traffic_control
