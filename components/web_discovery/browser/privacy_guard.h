/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PRIVACY_GUARD_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PRIVACY_GUARD_H_

#include <string>

#include "brave/components/web_discovery/browser/patterns.h"
#include "brave/components/web_discovery/browser/regex_util.h"
#include "url/gurl.h"

namespace web_discovery {

// Checks if a URL is likely to be private based on various criteria.
// If true, the page should not be investigated or reported.
bool IsPrivateURLLikely(RegexUtil& regex_util,
                        const GURL& url,
                        const PatternsURLDetails* matching_url_details);

// Determines if a search query is likely to contain private information.
// If true, the search query should not be investigated or reported.
bool IsPrivateQueryLikely(RegexUtil& regex_util, const std::string& query);

// Generates a simple search URL (without additional query parameters)
// based on the original search URL and query. Used for the double fetch
// to ensure that the user's profile is not involved in the query.
GURL GeneratePrivateSearchURL(const GURL& original_url,
                              const std::string& query,
                              const PatternsURLDetails& matching_url_details);

// Checks if a URL should be dropped due to its length or content.
// Currently only used for determining whether to mask a URL
// in the function below.
bool ShouldDropLongURL(RegexUtil& regex_util, const GURL& url);

// Masks a URL to protect privacy. Returns nullopt if URL is invalid.
std::optional<std::string> MaskURL(RegexUtil& regex_util, const GURL& url);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PRIVACY_GUARD_H_
