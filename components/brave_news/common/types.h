// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_TYPES_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_TYPES_H_

#include "base/containers/flat_map.h"
#include "brave/components/brave_news/common/brave_news.mojom-forward.h"
#include "third_party/abseil-cpp/absl/container/flat_hash_map.h"

namespace brave_news {

using Publishers = absl::flat_hash_map<std::string, mojom::PublisherPtr>;
using MojomPublishers = base::flat_map<std::string, mojom::PublisherPtr>;

MojomPublishers ConvertToMojomPublishers(const Publishers& publishers);
Publishers ClonePublishers(const Publishers& publishers);

}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_TYPES_H_
