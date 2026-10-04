/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/dayparts_database_table.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {


void Dayparts::Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE dayparts (
        campaign_id TEXT NOT NULL,
        days_of_week TEXT NOT NULL,
        start_minute INT NOT NULL,
        end_minute INT NOT NULL,
        PRIMARY KEY (
          campaign_id,
          days_of_week,
          start_minute,
          end_minute
        ) ON CONFLICT REPLACE
      ))");
}

void Dayparts::Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                       int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    default: {
      // No migration needed.
      break;
    }
  }
}

}  // namespace brave_ads::database::table
