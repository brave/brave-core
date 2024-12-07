/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table_util.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"

namespace brave_ads::database {

void PurgeExpiredAdEvents() {
  const database::table::AdEvents database_table;
  database_table.PurgeExpired(base::BindOnce([](bool success) {
    if (!success) {
      return BLOG(0, "Failed to purge expired ad events");
    }

    BLOG(3, "Successfully purged expired ad events");
  }));
}

void PurgeAllOrphanedAdEvents() {
  const database::table::AdEvents database_table;
  database_table.PurgeAllOrphaned(base::BindOnce([](bool success) {
    if (!success) {
      return BLOG(0, "Failed to purge all orphaned ad events");
    }

    BLOG(3, "Successfully purged all orphaned ad events");
  }));
}

}  // namespace brave_ads::database
