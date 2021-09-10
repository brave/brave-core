// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_FEED_PARSING_H_
#define BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_FEED_PARSING_H_

#include "base/values.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"

namespace brave_news {

bool ParseFeedItem(const base::Value& feed_item_raw,
                   mojom::FeedItemPtr* feed_item);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_TODAY_BROWSER_FEED_PARSING_H_
