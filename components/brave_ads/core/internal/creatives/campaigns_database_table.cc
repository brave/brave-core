/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/campaigns_database_table.h"

#include <cstddef>
#include <utility>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "campaigns";

size_t BindColumns(mojom::DBStatementInfo* mojom_statement,
                   const CreativeAdList& creative_ads) {
  CHECK(mojom_statement);
  CHECK(!creative_ads.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindColumnString(mojom_statement, index++, creative_ad.campaign_id);
    BindColumnTime(mojom_statement, index++, creative_ad.start_at);
    BindColumnTime(mojom_statement, index++, creative_ad.end_at);
    BindColumnInt(mojom_statement, index++, creative_ad.daily_cap);
    BindColumnString(mojom_statement, index++, creative_ad.advertiser_id);
    BindColumnInt(mojom_statement, index++, creative_ad.priority);
    BindColumnDouble(mojom_statement, index++, creative_ad.pass_through_rate);

    ++row_count;
  }

  return row_count;
}

}  // namespace

void Campaigns::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();

  DeleteTable(&*mojom_transaction, GetTableName());

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void Campaigns::Insert(mojom::DBTransactionInfo* mojom_transaction,
                       const CreativeAdList& creative_ads) {
  CHECK(mojom_transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type = mojom::DBStatementInfo::OperationType::kRun;
  mojom_statement->sql = BuildInsertSql(&*mojom_statement, creative_ads);
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

std::string Campaigns::GetTableName() const {
  return kTableName;
}

void Campaigns::Create(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  Execute(mojom_transaction, R"(
      CREATE TABLE campaigns (
        id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        start_at TIMESTAMP NOT NULL,
        end_at TIMESTAMP NOT NULL,
        daily_cap INTEGER DEFAULT 0 NOT NULL,
        advertiser_id TEXT NOT NULL,
        priority INTEGER NOT NULL DEFAULT 0,
        ptr DOUBLE NOT NULL DEFAULT 1
      );)");
}

void Campaigns::Migrate(mojom::DBTransactionInfo* mojom_transaction,
                        const int to_version) {
  CHECK(mojom_transaction);

  switch (to_version) {
    case 43: {
      MigrateToV43(mojom_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void Campaigns::MigrateToV43(
    mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // We can safely recreate the table because it will be repopulated after
  // downloading the catalog.
  DropTable(mojom_transaction, GetTableName());
  Create(mojom_transaction);
}

std::string Campaigns::BuildInsertSql(
    mojom::DBStatementInfo* mojom_statement,
    const CreativeAdList& creative_ads) const {
  CHECK(mojom_statement);
  CHECK(!creative_ads.empty());

  const size_t row_count = BindColumns(mojom_statement, creative_ads);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            id,
            start_at,
            end_at,
            daily_cap,
            advertiser_id,
            priority,
            ptr
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/7, row_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
