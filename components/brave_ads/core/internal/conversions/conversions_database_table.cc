/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/conversions_database_table.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "creative_ad_conversions";

void BindRecords(mojom::DBCommandInfo* command) {
  CHECK(command);

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // type
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // url_pattern
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // advertiser_public_key
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,  // observation_window
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE  // expire_at
  };
}

size_t BindParameters(mojom::DBCommandInfo* command,
                      const ConversionList& conversions) {
  CHECK(command);

  size_t count = 0;

  int index = 0;
  for (const auto& conversion : conversions) {
    BindString(command, index++, conversion.creative_set_id);
    BindString(command, index++, conversion.type);
    BindString(command, index++, conversion.url_pattern);
    BindString(command, index++, conversion.advertiser_public_key);
    BindInt(command, index++, conversion.observation_window.InDays());
    BindDouble(command, index++, conversion.expire_at.ToDoubleT());

    count++;
  }

  return count;
}

ConversionInfo GetFromRecord(mojom::DBRecordInfo* record) {
  CHECK(record);

  ConversionInfo conversion;

  conversion.creative_set_id = ColumnString(record, 0);
  conversion.type = ColumnString(record, 1);
  conversion.url_pattern = ColumnString(record, 2);
  conversion.advertiser_public_key = ColumnString(record, 3);
  conversion.observation_window = base::Days(ColumnInt(record, 4));
  conversion.expire_at = base::Time::FromDoubleT(ColumnDouble(record, 5));

  return conversion;
}

void OnGetConversions(GetConversionsCallback callback,
                      mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get creative conversions");
    return std::move(callback).Run(/*success*/ false,
                                   /*conversion_queue_items*/ {});
  }

  ConversionList conversions;

  for (const auto& record : command_response->result->get_records()) {
    const ConversionInfo conversion = GetFromRecord(&*record);
    conversions.push_back(conversion);
  }

  std::move(callback).Run(/*success*/ true, conversions);
}

void MigrateToV23(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  DropTable(transaction, "ad_conversions");

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE IF NOT EXISTS creative_ad_conversions (creative_set_id "
      "TEXT NOT NULL, type TEXT NOT NULL, url_pattern TEXT NOT NULL, "
      "advertiser_public_key TEXT, observation_window INTEGER NOT NULL, "
      "expiry_timestamp TIMESTAMP NOT NULL, UNIQUE(creative_set_id, type) ON "
      "CONFLICT REPLACE, PRIMARY KEY(creative_set_id, type));";
  transaction->commands.push_back(std::move(command));
}

void MigrateToV28(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  // Create a temporary table with renamed |expire_at| column.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE creative_ad_conversions_temp (creative_set_id TEXT NOT "
      "NULL, type TEXT NOT NULL, url_pattern TEXT NOT NULL, "
      "advertiser_public_key TEXT, observation_window INTEGER NOT NULL, "
      "expire_at TIMESTAMP NOT NULL, UNIQUE(creative_set_id, type) ON CONFLICT "
      "REPLACE, PRIMARY KEY(creative_set_id, type));";
  transaction->commands.push_back(std::move(command));

  // Copy columns to temporary table.
  const std::vector<std::string> from_columns = {
      "creative_set_id",    "type",
      "url_pattern",        "advertiser_public_key",
      "observation_window", "expiry_timestamp"};

  const std::vector<std::string> to_columns = {
      "creative_set_id",    "type",     "url_pattern", "advertiser_public_key",
      "observation_window", "expire_at"};

  CopyTableColumns(transaction, "creative_ad_conversions",
                   "creative_ad_conversions_temp", from_columns, to_columns,
                   /*should_drop*/ true);

  // Rename temporary table.
  RenameTable(transaction, "creative_ad_conversions_temp",
              "creative_ad_conversions");
}

}  // namespace

void Conversions::Save(const ConversionList& conversions,
                       ResultCallback callback) {
  if (conversions.empty()) {
    return std::move(callback).Run(/*success*/ true);
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  InsertOrUpdate(&*transaction, conversions);

  RunTransaction(std::move(transaction), std::move(callback));
}

void Conversions::GetAll(GetConversionsCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
      "SELECT ac.creative_set_id, ac.type, ac.url_pattern, "
      "ac.advertiser_public_key, ac.observation_window, ac.expire_at FROM $1 "
      "AS ac WHERE $2 < expire_at;",
      {GetTableName(), TimeAsTimestampString(base::Time::Now())}, nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnGetConversions, std::move(callback)));
}

void Conversions::PurgeExpired(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      "DELETE FROM $1 WHERE $2 >= expire_at;",
      {GetTableName(), TimeAsTimestampString(base::Time::Now())}, nullptr);
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

std::string Conversions::GetTableName() const {
  return kTableName;
}

void Conversions::Create(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE creative_ad_conversions (creative_set_id "
      "TEXT NOT NULL, type TEXT NOT NULL, url_pattern TEXT NOT NULL, "
      "advertiser_public_key TEXT, observation_window INTEGER NOT NULL, "
      "expire_at TIMESTAMP NOT NULL, UNIQUE(creative_set_id, type) ON CONFLICT "
      "REPLACE, PRIMARY KEY(creative_set_id, type));";
  transaction->commands.push_back(std::move(command));
}

void Conversions::Migrate(mojom::DBTransactionInfo* transaction,
                          const int to_version) {
  CHECK(transaction);

  switch (to_version) {
    case 23: {
      MigrateToV23(transaction);
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

void Conversions::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                                 const ConversionList& conversions) {
  CHECK(transaction);

  if (conversions.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = BuildInsertOrUpdateSql(&*command, conversions);
  transaction->commands.push_back(std::move(command));
}

std::string Conversions::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const ConversionList& conversions) const {
  CHECK(command);

  const size_t binded_parameters_count = BindParameters(command, conversions);

  return base::ReplaceStringPlaceholders(
      "INSERT OR REPLACE INTO $1 (creative_set_id, type, url_pattern, "
      "advertiser_public_key, observation_window, expire_at) VALUES $2;",
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count*/ 6, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
