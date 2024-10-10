// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_URLS_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_URLS_H_

#include <string>

namespace brave_news {

inline constexpr char kRegionUrlPart[] = "global.";

std::string GetHostname();
std::string GetMatchingPCDNHostname();

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_URLS_H_
