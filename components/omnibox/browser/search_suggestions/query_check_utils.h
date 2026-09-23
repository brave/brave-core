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

// Returns true if `query` has more than 7 digits once non-alphanumerics are
// removed. Intention here is to prevent PII (card numbers, phone numbers, etc)
// from being sent via the search suggestions API.
//
// Called by `IsSuspiciousQuery` but needs to be public so we can provide local
// calculations as a fallback when query is flagged as suspicious. For more
// info, see `BraveSearchProvider::MaybeEvaluateLocally`.
bool HasLongNumberInQuery(const std::string& query);

}  // namespace search_suggestions

#endif  // BRAVE_COMPONENTS_OMNIBOX_BROWSER_SEARCH_SUGGESTIONS_QUERY_CHECK_UTILS_H_
