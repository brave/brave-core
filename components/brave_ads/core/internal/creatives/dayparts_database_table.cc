/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/dayparts_database_table.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "dayparts";

size_t BindParameters(mojom::DBCommandInfo* command,
                      const CreativeAdList& creative_ads) {
  CHECK(command);

  size_t count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    for (const auto& daypart : creative_ad.dayparts) {
      BindString(command, index++, creative_ad.campaign_id);
      BindString(command, index++, daypart.days_of_week);
      BindInt(command, index++, daypart.start_minute);
      BindInt(command, index++, daypart.end_minute);

      count++;
    }
  }

  return count;
}

void MigrateToV24(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  DropTable(transaction, "dayparts");

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE dayparts (campaign_id TEXT NOT NULL, dow TEXT NOT NULL, "
      "start_minute INT NOT NULL, end_minute INT NOT NULL, PRIMARY KEY "
      "(campaign_id, dow, start_minute, end_minute), UNIQUE(campaign_id, dow, "
      "start_minute, end_minute) ON CONFLICT REPLACE);";
  transaction->commands.push_back(std::move(command));
}

void MigrateToV28(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  // Create a temporary table with renamed |expire_at| column.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE dayparts_temp (campaign_id TEXT NOT NULL, days_of_week "
      "TEXT NOT NULL, start_minute INT NOT NULL, end_minute INT NOT NULL, "
      "PRIMARY KEY (campaign_id, days_of_week, start_minute, end_minute), "
      "UNIQUE(campaign_id, days_of_week, start_minute, end_minute) ON CONFLICT "
      "REPLACE);";
  transaction->commands.push_back(std::move(command));

  // Copy columns to temporary table.
  const std::vector<std::string> from_columns = {"campaign_id", "dow",
                                                 "start_minute", "end_minute"};

  const std::vector<std::string> to_columns = {"campaign_id", "days_of_week",
                                               "start_minute", "end_minute"};

  CopyTableColumns(transaction, "dayparts", "dayparts_temp", from_columns,
                   to_columns,
                   /*should_drop*/ true);

  // Rename temporary table.
  RenameTable(transaction, "dayparts_temp", "dayparts");
}

}  // namespace

void Dayparts::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
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

void Dayparts::Delete(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  DeleteTable(&*transaction, GetTableName());

  RunTransaction(std::move(transaction), std::move(callback));
}

std::string Dayparts::GetTableName() const {
  return kTableName;
}

void Dayparts::Create(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE dayparts (campaign_id TEXT NOT NULL, "
      "days_of_week TEXT NOT NULL, start_minute INT NOT NULL, end_minute INT "
      "NOT NULL, PRIMARY KEY (campaign_id, days_of_week, start_minute, "
      "end_minute), UNIQUE(campaign_id, days_of_week, start_minute, "
      "end_minute) ON CONFLICT REPLACE);";
  transaction->commands.push_back(std::move(command));
}

void Dayparts::Migrate(mojom::DBTransactionInfo* transaction,
                       const int to_version) {
  CHECK(transaction);

  switch (to_version) {
    case 24: {
      MigrateToV24(transaction);
      break;
    }

    case 28: {
      MigrateToV28(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

std::string Dayparts::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const CreativeAdList& creative_ads) const {
  CHECK(command);

  const size_t binded_parameters_count = BindParameters(command, creative_ads);

  return base::ReplaceStringPlaceholders(
      "INSERT OR REPLACE INTO $1 (campaign_id, days_of_week, start_minute, "
      "end_minute) VALUES $2;",
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count*/ 4, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
