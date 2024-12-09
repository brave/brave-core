// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_DIFF_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_DIFF_H_

#include "base/values.h"
#include "brave/components/brave_news/common/mojom/brave_news.mojom-forward.h"

namespace brave_news::mojom {

base::Value Diff(const StatePtr& old_value, const StatePtr& new_value);

base::Value Diff(const ConfigurationPtr& old_value, const ConfigurationPtr& new_value);

base::Value Diff(const ChannelPtr& old_value, const ChannelPtr& new_value);

base::Value Diff(const PublisherPtr& old_value, const PublisherPtr& new_value);

}  // namespace brave_news::mojom

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_COMMON_DIFF_H_
