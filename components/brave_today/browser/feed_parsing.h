// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENT_BRAVE_TODAY_BROWSER_FEED_PARSING_H_
#define BRAVE_COMPONENT_BRAVE_TODAY_BROWSER_FEED_PARSING_H_

#include <string>
#include <unordered_set>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/brave_today/common/brave_news.mojom-forward.h"
#include "brave/components/brave_today/common/brave_news.mojom.h"

namespace brave_news {
using Publishers = base::flat_map<std::string, mojom::PublisherPtr>;
bool ParsePublisherList(const std::string& json,
                        Publishers* publishers);
bool ParseFeed(const std::string& json,
                        Publishers* publishers,
                        std::unordered_set<std::string>& history_hosts,
                        mojom::Feed* feed);
}  // namespace brave_news

#endif  // BRAVE_COMPONENT_BRAVE_TODAY_BROWSER_FEED_PARSING_H_
