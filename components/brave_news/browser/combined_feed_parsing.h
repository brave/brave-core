// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_COMBINED_FEED_PARSING_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_COMBINED_FEED_PARSING_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_news/common/brave_news.mojom.h"

namespace brave_news {

// Convert from the "combined feed" hosted remotely to Brave News mojom items.
std::vector<mojom::FeedItemPtr> ParseFeedItems(const base::Value& value);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_COMBINED_FEED_PARSING_H_
