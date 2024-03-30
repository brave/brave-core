// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_BUILDING_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_BUILDING_H_

#include <string>
#include <unordered_set>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/channels_controller.h"
#include "brave/components/brave_news/browser/publishers_parsing.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"
#include "components/prefs/pref_service.h"

namespace brave_news {

bool BuildFeed(const std::vector<mojom::FeedItemPtr>& feed_items,
               const std::unordered_set<std::string>& history_hosts,
               Publishers* publishers,
               mojom::Feed* feed,
               const BraveNewsSubscriptions& subscriptions);

// Exposed for testing
bool ShouldDisplayFeedItem(const mojom::FeedItemPtr& feed_item,
                           const Publishers* publishers,
                           const Channels& channels);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_FEED_BUILDING_H_
