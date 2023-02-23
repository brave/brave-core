// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_URLS_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_URLS_H_

#include <string>

namespace brave_news {

std::string GetHostname();

// Gets the version one region URL part string. This only includes the language.
std::string GetV1RegionUrlPart();

// Gets the region URL part string. In V2 this is 'global' and in V1, just the
// lang (i.e. 'en' or 'ja').
std::string GetRegionUrlPart();

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_URLS_H_
