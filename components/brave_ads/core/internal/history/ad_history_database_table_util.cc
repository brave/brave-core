/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/history/ad_history_database_table_util.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_database_table.h"
#include "brave/components/brave_ads/core/public/history/ad_history_item_info.h"

namespace brave_ads::database {

void PurgeExpiredAdHistory() {
  const database::table::AdHistory database_table;
  database_table.PurgeExpired(base::BindOnce([](bool success) {
    if (!success) {
      return BLOG(0, "Failed to purge expired ad history");
    }

    BLOG(3, "Successfully purged expired ad history");
  }));
}

void SaveAdHistory(const AdHistoryList& ad_history) {
  database::table::AdHistory database_table;
  database_table.Save(ad_history, base::BindOnce([](bool success) {
                        if (!success) {
                          return BLOG(0, "Failed to save ad history");
                        }

                        BLOG(3, "Successfully saved ad history");
                      }));
}

}  // namespace brave_ads::database
