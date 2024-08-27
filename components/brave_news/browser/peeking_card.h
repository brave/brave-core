// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_PEEKING_CARD_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_PEEKING_CARD_H_

#include "brave/components/brave_news/browser/feed_fetcher.h"
#include "brave/components/brave_news/browser/feed_sampling.h"
#include "brave/components/brave_news/common/subscriptions_snapshot.h"

namespace brave_news {

std::optional<size_t> GetPeekingCard(const SubscriptionsSnapshot& subscriptions,
                                     const ArticleInfos& articles);

}

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_PEEKING_CARD_H_
