/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/feed/feed_feature_list.cc"

#include "base/feature_override.h"
#include "build/build_config.h"

namespace feed {

OVERRIDE_FEATURE_DEFAULT_STATES({{
#if BUILDFLAG(IS_ANDROID)
    {kFeedContainment, base::FEATURE_DISABLED_BY_DEFAULT},
    {kInterestFeedV2, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
}});

}  // namespace feed
