/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_BRAVE_SEARCH_COUNTRY_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_BRAVE_SEARCH_COUNTRY_H_

#include <string>

class PrefService;

namespace brave_search {

// Returns the ISO 3166-1 alpha-2 country code (e.g., "US", "ZA", "DE")
// derived from the browser's country detection. Uses the following priority:
// 1. User-configured Brave Search country preference (if set)
// 2. countryid_at_install from browser preferences
// 3. OS locale as fallback
// Returns empty string if country cannot be determined.
std::string GetBraveSearchCountryCode(PrefService* prefs);

// Converts a countryid integer (as stored in countryid_at_install) to
// a two-letter ISO 3166-1 alpha-2 country code string.
// The countryid is encoded as two ASCII characters packed into an integer:
// e.g., 'U' * 256 + 'S' = 21843 => "US"
//       'Z' * 256 + 'A' = 23105 => "ZA"
std::string CountryIdToCountryCode(int country_id);

}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_BRAVE_SEARCH_COUNTRY_H_
