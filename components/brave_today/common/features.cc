/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_today/common/features.h"

#include "base/feature_list.h"

namespace brave_today {
namespace features {

BASE_FEATURE(kBraveNewsFeature, "BraveNews", base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kBraveNewsV2Feature,
             "BraveNewsV2",
#if BUILDFLAG(IS_ANDROID)
             base::FEATURE_DISABLED_BY_DEFAULT
#else
             base::FEATURE_ENABLED_BY_DEFAULT
#endif
);

BASE_FEATURE(kBraveNewsCardPeekFeature,
             "BraveNewsCardPeek",
             base::FEATURE_ENABLED_BY_DEFAULT);
BASE_FEATURE(kBraveNewsSubscribeButtonFeature,
             "BraveNewsSubscribeButton",
             base::FEATURE_DISABLED_BY_DEFAULT);

}  // namespace features
}  // namespace brave_today
