/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "../../../../components/feed/feed_feature_list.cc"

#include "base/feature_override.h"

namespace feed {

#if defined(OS_ANDROID)
DISABLE_FEATURE_BY_DEFAULT(kInterestFeedContentSuggestions);
DISABLE_FEATURE_BY_DEFAULT(kInterestFeedV2);
#endif

}  // namespace feed
