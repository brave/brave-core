/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/random/random_util.h"

#include "base/time/time.h"
#include "brave_base/random.h"

namespace brave_ads {

base::TimeDelta RandTimeDelta(const base::TimeDelta time_delta) {
  return base::Seconds(brave_base::random::Geometric(time_delta.InSecondsF()));
}

}  // namespace brave_ads
