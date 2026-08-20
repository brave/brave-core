/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_BRAVE_SEARCH_URL_PROCESSOR_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_BRAVE_SEARCH_URL_PROCESSOR_H_

#include <string>

class PrefService;

namespace brave_search {

// Placeholder used in Brave Search URL templates for the country parameter.
inline constexpr char kBraveCountryPlaceholder[] = "{brave:country}";

// Processes a Brave Search URL by replacing the {brave:country} placeholder
// with the user's actual country code derived from browser preferences.
// If the URL does not contain the placeholder, it is returned unchanged.
// If the country code cannot be determined, the placeholder (and its
// preceding &country= parameter) is removed to let the server decide.
std::string ProcessBraveSearchUrl(const std::string& url, PrefService* prefs);

}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_COMMON_BRAVE_SEARCH_URL_PROCESSOR_H_
