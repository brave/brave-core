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

bool IsPrivateURLLikely(RegexUtil& regex_util,
                        const GURL& url,
                        const PatternsURLDetails* matching_url_details);

bool IsPrivateQueryLikely(RegexUtil& regex_util, const std::string& query);

GURL GeneratePrivateSearchURL(const GURL& original_url,
                              const std::string& query,
                              const PatternsURLDetails& matching_url_details);

bool ShouldDropLongURL(RegexUtil& regex_util, const GURL& url);

std::optional<std::string> MaskURL(RegexUtil& regex_util, const GURL& url);

}  // namespace web_discovery

#endif  // BRAVE_COMPONENTS_WEB_DISCOVERY_BROWSER_PRIVACY_GUARD_H_
