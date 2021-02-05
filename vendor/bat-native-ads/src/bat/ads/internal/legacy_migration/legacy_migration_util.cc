/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/legacy_migration_util.h"

#include "base/time/time.h"

namespace ads {

uint64_t MigrateTimestampToDoubleT(const uint64_t timestamp_in_seconds) {
  if (timestamp_in_seconds < 10000000000) {
    // Already migrated as DoubleT will never reach 10000000000 in our lifetime
    // and legacy timestamps are above 10000000000
    return timestamp_in_seconds;
  }

  // Migrate date to DoubleT
  const base::Time now = base::Time::Now();

  const uint64_t now_in_seconds =
      static_cast<uint64_t>((now - base::Time()).InSeconds());

  const uint64_t delta = timestamp_in_seconds - now_in_seconds;

  const base::Time time = now + base::TimeDelta::FromSeconds(delta);

  return static_cast<uint64_t>(time.ToDoubleT());
}

}  // namespace ads
