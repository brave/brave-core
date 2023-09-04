/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_news/common/features.h"

#include "base/feature_list.h"

namespace brave_news::features {

BASE_FEATURE(kBraveNewsCardPeekFeature,
             "BraveNewsCardPeek",
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveNewsFeedUpdate,
             "BraveNewsFeedUpdate",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace brave_news::features
