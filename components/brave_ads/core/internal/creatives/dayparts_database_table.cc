/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/dayparts_database_table.h"

#include <cstddef>
#include <utility>

#include "base/check.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "dayparts";

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const CreativeAdList& creative_ads) {
  CHECK(mojom_db_action);
  CHECK(!creative_ads.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    for (const auto& daypart : creative_ad.dayparts) {
      BindColumnString(mojom_db_action, index++, creative_ad.campaign_id);
      BindColumnString(mojom_db_action, index++, daypart.days_of_week);
      BindColumnInt(mojom_db_action, index++, daypart.start_minute);
      BindColumnInt(mojom_db_action, index++, daypart.end_minute);

      ++row_count;
    }
  }

  return row_count;
}

}  // namespace

void Dayparts::Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                      const CreativeAdList& creative_ads) {
  CHECK(mojom_db_transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, creative_ads);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

void Dayparts::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  DeleteTable(mojom_db_transaction, GetTableName());

  RunDBTransaction(FROM_HERE, std::move(mojom_db_transaction),
                   std::move(callback));
}

std::string Dayparts::GetTableName() const {
  return kTableName;
}

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
      );)");
}

void Dayparts::Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                       int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 46: {
      MigrateToV46(mojom_db_transaction);
      break;
    }

    default: {
      // No migration needed.
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void Dayparts::MigrateToV46(
    const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // We can safely recreate the table because it will be repopulated after
  // downloading the catalog.
  DropTable(mojom_db_transaction, GetTableName());
  Create(mojom_db_transaction);
}

std::string Dayparts::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const CreativeAdList& creative_ads) const {
  CHECK(mojom_db_action);
  CHECK(!creative_ads.empty());

  const size_t row_count = BindColumns(mojom_db_action, creative_ads);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            campaign_id,
            days_of_week,
            start_minute,
            end_minute
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/4, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
