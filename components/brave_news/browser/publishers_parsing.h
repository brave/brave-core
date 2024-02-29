// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_PUBLISHERS_PARSING_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_PUBLISHERS_PARSING_H_

#include <optional>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_news/browser/brave_news_pref_manager.h"
#include "brave/components/brave_news/browser/publishers_controller.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"

namespace brave_news {

std::optional<Publishers> ParseCombinedPublisherList(const base::Value& value);

void ParseDirectPublisherList(const std::vector<DirectFeed>& direct_feeds,
                              std::vector<mojom::PublisherPtr>* publishers);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_PUBLISHERS_PARSING_H_
