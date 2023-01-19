/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/deposits/deposits_database_util.h"

#include "base/functional/bind.h"
#include "bat/ads/internal/account/deposits/deposits_database_table.h"
#include "bat/ads/internal/common/logging_util.h"

namespace ads::database {

void PurgeExpiredDeposits() {
  const database::table::Deposits database_table;
  database_table.PurgeExpired(base::BindOnce([](const bool success) {
    if (!success) {
      BLOG(0, "Failed to purge expired deposits");
      return;
    }

    BLOG(3, "Successfully purged expired deposits");
  }));
}

}  // namespace ads::database
