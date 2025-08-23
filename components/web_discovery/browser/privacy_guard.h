/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PRIVACY_GUARD_H_
#define BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PRIVACY_GUARD_H_

#include <string>

#include "url/gurl.h"

namespace web_discovery {

// Determines if a search query is likely to contain private information.
// If true, the search query should not be investigated or reported.
bool IsPrivateQueryLikely(std::string_view query);

// Generates a simple search URL (without additional query parameters)
// based on the original search URL and query. Used for the double fetch
// to ensure that the user's profile is not involved in the query.
GURL GeneratePrivateSearchURL(const GURL& original_url,
                              std::string_view query,
                              std::optional<std::string_view> prefix);

// Checks if a URL should be dropped entirely due to security/privacy concerns.
bool ShouldDropURL(const GURL& url);

// Checks if a URL should be masked/truncated due to its length or content.
bool ShouldMaskURL(const GURL& url);

// Masks a URL to protect privacy. Returns nullopt if URL should be dropped.
// Replaces the URL path with a placeholder token, if applicable.
std::optional<std::string> MaskURL(const GURL& url, bool relaxed);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PRIVACY_GUARD_H_
