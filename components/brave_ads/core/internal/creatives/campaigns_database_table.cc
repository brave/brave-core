/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/campaigns_database_table.h"

#include <cstddef>
#include <utility>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "campaigns";

size_t BindParameters(mojom::DBCommandInfo* command,
                      const CreativeAdList& creative_ads) {
  CHECK(command);

  size_t count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindString(command, index++, creative_ad.campaign_id);
    BindInt64(command, index++,
              creative_ad.start_at.ToDeltaSinceWindowsEpoch().InMicroseconds());
    BindInt64(command, index++,
              creative_ad.end_at.ToDeltaSinceWindowsEpoch().InMicroseconds());
    BindInt(command, index++, creative_ad.daily_cap);
    BindString(command, index++, creative_ad.advertiser_id);
    BindInt(command, index++, creative_ad.priority);
    BindDouble(command, index++, creative_ad.pass_through_rate);

    ++count;
  }

  return count;
}

void MigrateToV29(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  DropTable(transaction, "campaigns");

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE campaigns (campaign_id TEXT NOT NULL PRIMARY KEY UNIQUE ON "
      "CONFLICT REPLACE, start_at TIMESTAMP NOT NULL, end_at TIMESTAMP NOT "
      "NULL, daily_cap INTEGER DEFAULT 0 NOT NULL, advertiser_id TEXT NOT "
      "NULL, priority INTEGER NOT NULL DEFAULT 0, ptr DOUBLE NOT NULL DEFAULT "
      "1);";
  transaction->commands.push_back(std::move(command));
}

}  // namespace

void Campaigns::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(&*transaction, GetTableName());

  RunTransaction(std::move(transaction), std::move(callback));
}

void Campaigns::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                               const CreativeAdList& creative_ads) {
  CHECK(transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = BuildInsertOrUpdateSql(&*command, creative_ads);
  transaction->commands.push_back(std::move(command));
}

std::string Campaigns::GetTableName() const {
  return kTableName;
}

void Campaigns::Create(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE campaigns (campaign_id TEXT NOT NULL PRIMARY KEY UNIQUE ON "
      "CONFLICT REPLACE, start_at TIMESTAMP NOT NULL, end_at TIMESTAMP NOT "
      "NULL, daily_cap INTEGER DEFAULT 0 NOT NULL, advertiser_id TEXT NOT "
      "NULL, priority INTEGER NOT NULL DEFAULT 0, ptr DOUBLE NOT NULL DEFAULT "
      "1);";
  transaction->commands.push_back(std::move(command));
}

void Campaigns::Migrate(mojom::DBTransactionInfo* transaction,
                        const int to_version) {
  CHECK(transaction);

  switch (to_version) {
    case 29: {
      MigrateToV29(transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

std::string Campaigns::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const CreativeAdList& creative_ads) const {
  CHECK(command);

  const size_t binded_parameters_count = BindParameters(command, creative_ads);

  return base::ReplaceStringPlaceholders(
      "INSERT OR REPLACE INTO $1 (campaign_id, start_at, end_at, daily_cap, "
      "advertiser_id, priority, ptr) VALUES $2;",
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count=*/7, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
