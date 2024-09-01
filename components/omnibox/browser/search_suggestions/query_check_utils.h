// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OMNIBOX_BROWSER_SEARCH_SUGGESTIONS_QUERY_CHECK_UTILS_H_
#define BRAVE_COMPONENTS_OMNIBOX_BROWSER_SEARCH_SUGGESTIONS_QUERY_CHECK_UTILS_H_

#include <string>

namespace search_suggestions {

bool IsSuspiciousQuery(const std::string& query);
bool IsSafeQueryUrl(const std::string& query);

}  // namespace search_suggestions

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_SEARCH_SUGGESTIONS_QUERY_CHECK_UTILS_H_
