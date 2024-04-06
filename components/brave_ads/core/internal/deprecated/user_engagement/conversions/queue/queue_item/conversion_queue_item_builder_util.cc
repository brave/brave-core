/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/user_engagement/conversions/queue/queue_item/conversion_queue_item_builder_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/random/random_util.h"
#include "brave/components/brave_ads/core/internal/flags/debug/debug_flag_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_feature.h"

namespace brave_ads {

namespace {
constexpr base::TimeDelta kDebugProcessConversionAfter = base::Minutes(1);
}  // namespace

base::Time ProcessConversionAt() {
  return base::Time::Now() +
         (ShouldDebug() ? kDebugProcessConversionAfter
                        : RandTimeDelta(kProcessConversionAfter.Get()));
}

}  // namespace brave_ads
