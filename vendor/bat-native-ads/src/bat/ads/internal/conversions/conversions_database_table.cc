/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/conversions/conversions_database_table.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/database/database_bind_util.h"
#include "bat/ads/internal/common/database/database_column_util.h"
#include "bat/ads/internal/common/database/database_table_util.h"
#include "bat/ads/internal/common/database/database_transaction_util.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/common/time/time_formatting_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads::database::table {

namespace {

constexpr char kTableName[] = "creative_ad_conversions";

int BindParameters(mojom::DBCommandInfo* command,
                   const ConversionList& conversions) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& conversion : conversions) {
    BindString(command, index++, conversion.creative_set_id);
    BindString(command, index++, conversion.type);
    BindString(command, index++, conversion.url_pattern);
    BindString(command, index++, conversion.advertiser_public_key);
    BindInt(command, index++, conversion.observation_window);
    BindDouble(command, index++, conversion.expire_at.ToDoubleT());

    count++;
  }

  return count;
}

ConversionInfo GetFromRecord(mojom::DBRecordInfo* record) {
  DCHECK(record);

  ConversionInfo conversion;

  conversion.creative_set_id = ColumnString(record, 0);
  conversion.type = ColumnString(record, 1);
  conversion.url_pattern = ColumnString(record, 2);
  conversion.advertiser_public_key = ColumnString(record, 3);
  conversion.observation_window = ColumnInt(record, 4);
  conversion.expire_at = base::Time::FromDoubleT(ColumnDouble(record, 5));

  return conversion;
}

void OnGetConversions(GetConversionsCallback callback,
                      mojom::DBCommandResponseInfoPtr response) {
  if (!response || response->status !=
                       mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get creative conversions");
    std::move(callback).Run(/*success*/ false, {});
    return;
  }

  ConversionList conversions;

  for (const auto& record : response->result->get_records()) {
    const ConversionInfo conversion = GetFromRecord(record.get());
    conversions.push_back(conversion);
  }

  std::move(callback).Run(/*success*/ true, conversions);
}

void MigrateToV23(mojom::DBTransactionInfo* transaction) {
  DCHECK(transaction);

  DropTable(transaction, "ad_conversions");

  const std::string query =
      "CREATE TABLE IF NOT EXISTS creative_ad_conversions "
      "(creative_set_id TEXT NOT NULL, "
      "type TEXT NOT NULL, "
      "url_pattern TEXT NOT NULL, "
      "advertiser_public_key TEXT, "
      "observation_window INTEGER NOT NULL, "
      "expiry_timestamp TIMESTAMP NOT NULL, "
      "UNIQUE(creative_set_id, type) ON CONFLICT REPLACE, "
      "PRIMARY KEY(creative_set_id, type))";

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace

void Conversions::Save(const ConversionList& conversions,
                       ResultCallback callback) {
  if (conversions.empty()) {
    std::move(callback).Run(/*success*/ true);
    return;
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  InsertOrUpdate(transaction.get(), conversions);

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void Conversions::GetAll(GetConversionsCallback callback) const {
  const std::string query = base::StringPrintf(
      "SELECT "
      "ac.creative_set_id, "
      "ac.type, "
      "ac.url_pattern, "
      "ac.advertiser_public_key, "
      "ac.observation_window, "
      "ac.expiry_timestamp "
      "FROM %s AS ac "
      "WHERE %s < expiry_timestamp",
      GetTableName().c_str(), TimeAsTimestampString(base::Time::Now()).c_str());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // type
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // url_pattern
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // advertiser_public_key
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE,  // observation_window
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE  // expire_at
  };

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnGetConversions, std::move(callback)));
}

void Conversions::PurgeExpired(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  const std::string query = base::StringPrintf(
      "DELETE FROM %s "
      "WHERE %s >= expiry_timestamp",
      GetTableName().c_str(), TimeAsTimestampString(base::Time::Now()).c_str());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

std::string Conversions::GetTableName() const {
  return kTableName;
}

void Conversions::Migrate(mojom::DBTransactionInfo* transaction,
                          const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 23: {
      MigrateToV23(transaction);
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
  DCHECK(transaction);

  if (conversions.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), conversions);

  transaction->commands.push_back(std::move(command));
}

std::string Conversions::BuildInsertOrUpdateQuery(
    mojom::DBCommandInfo* command,
    const ConversionList& conversions) const {
  DCHECK(command);

  const int count = BindParameters(command, conversions);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(creative_set_id, "
      "type, "
      "url_pattern, "
      "advertiser_public_key, "
      "observation_window, "
      "expiry_timestamp) VALUES %s",
      GetTableName().c_str(),
      BuildBindingParameterPlaceholders(6, count).c_str());
}

}  // namespace ads::database::table
