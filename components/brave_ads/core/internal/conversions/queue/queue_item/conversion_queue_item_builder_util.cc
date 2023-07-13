/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_builder_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/flags/debug/debug_flag_util.h"
#include "brave/components/brave_ads/core/internal/privacy/random/random_util.h"

namespace brave_ads {

namespace {

constexpr base::TimeDelta kProcessQueueItemAfter = base::Days(1);
constexpr base::TimeDelta kDebugProcessQueueItemAfter = base::Minutes(1);

}  // namespace

base::Time ProcessQueueItemAt() {
  return base::Time::Now() +
         (ShouldDebug() ? kDebugProcessQueueItemAfter
                        : privacy::RandTimeDelta(kProcessQueueItemAfter));
}

}  // namespace brave_ads
