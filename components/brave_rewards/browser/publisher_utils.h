/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_PUBLISHER_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_PUBLISHER_UTILS_H_

#include <optional>
#include <string>

#include "url/gurl.h"

namespace brave_rewards {

// Returns the publisher ID associated with the specified URL, or `nullopt` if
// the publisher ID cannot be statically determined from the URL. For example,
// a `nullopt` will be returned if the URL points to a configured social media
// platform where multiple publishers can be registered.
std::optional<std::string> GetPublisherIdFromURL(const GURL& url);

// Returns the publisher domain for the specified URL. For social media
// platforms, the site domain will be returned (e.g "twitter.com").
std::optional<std::string> GetPublisherDomainFromURL(const GURL& url);

// Returns a value indicating whether content scripting is used to measure AC.
bool IsAutoContributeHandledByContentScript(const GURL& url);

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_PUBLISHER_UTILS_H_
