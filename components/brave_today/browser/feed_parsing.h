// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_FEED_PARSING_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_FEED_PARSING_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"

namespace brave_news {

// Convert from the "combined feed" hosted remotely to Brave News mojom items.
// TODO(petemill): Rename this file to combined_feed_parsing.h or similar,
// in order to differentiate the "Combined Feed" from
// a "Direct Feed" (a.k.a RSS).
bool ParseFeedItems(const base::Value& json_value,
                    std::vector<mojom::FeedItemPtr>* feed_items);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_FEED_PARSING_H_
