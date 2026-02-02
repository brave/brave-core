// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_LOCALES_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_LOCALES_HELPER_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"

namespace brave_news {

using Publishers = base::flat_map<std::string, mojom::PublisherPtr>;

// Gets a list of all available locales from the publishers.
base::flat_set<std::string> GetPublisherLocales(const Publishers& publishers);

// Attempts to get a minimal set of locales for all the publishers & channels
// the user is subscribed to.
// This is implemented using a greedy algorithm which adds the most commonly
// occurring locale we aren't going to fetch until all publishers will be
// fetched.
// As such, it isn't guaranteed to be the absolutely minimum set of locales, but
// should work well enough for our purposes.
// Complexity is O(subscribed_publishers * subscribed_locales).
base::flat_set<std::string> GetMinimalLocalesSet(
    base::flat_set<std::string> channel_locales,
    const Publishers& publishers);

// Calculate if Brave News should be enabled on the NTP by checking the
// user's locale.
bool IsUserInDefaultEnabledLocale();
}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_LOCALES_HELPER_H_
