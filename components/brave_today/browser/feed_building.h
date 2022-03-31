// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_FEED_BUILDING_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_FEED_BUILDING_H_

#include <string>
#include <unordered_set>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_today/browser/publishers_parsing.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"

namespace brave_news {

bool BuildFeed(const std::vector<mojom::FeedItemPtr>& feed_items,
               const std::unordered_set<std::string>& history_hosts,
               Publishers* publishers,
               mojom::Feed* feed);

// Exposed for testing
bool ShouldDisplayFeedItem(const mojom::FeedItemPtr& feed_item,
                           const Publishers* publishers);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_FEED_BUILDING_H_
