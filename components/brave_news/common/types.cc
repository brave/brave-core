// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_news/common/types.h"

#include <utility>
#include <vector>

#include "brave/components/brave_news/common/brave_news.mojom.h"

namespace brave_news {

MojomPublishers ConvertToMojomPublishers(const Publishers& publishers) {
  std::vector<std::pair<std::string, mojom::PublisherPtr>> pairs;
  pairs.reserve(publishers.size());

  for (auto& [key, value_ptr] : publishers) {
    pairs.emplace_back(key, value_ptr.Clone());
  }

  // Create a flat map using O(N log N) ctor instead of O(N^2) insertion.
  return MojomPublishers(std::move(pairs));
}

Publishers ClonePublishers(const Publishers& publishers) {
  Publishers publishers_copy;
  publishers_copy.reserve(publishers.size());

  for (const auto& [key, value] : publishers) {
    publishers_copy.emplace(key, value.Clone());
  }

  return publishers_copy;
}

}  // namespace brave_news
