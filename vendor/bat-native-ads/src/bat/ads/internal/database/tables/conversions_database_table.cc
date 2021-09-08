/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/conversions_database_table.h"

#include <cstdint>
#include <functional>
#include <utility>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time_formatting_util.h"

namespace ads {
namespace database {
namespace table {

namespace {
const char kTableName[] = "creative_ad_conversions";
}  // namespace

Conversions::Conversions() = default;

Conversions::~Conversions() = default;

void Conversions::Save(const ConversionList& conversions,
                       ResultCallback callback) {
  if (conversions.empty()) {
    callback(/* success */ true);
    return;
  }

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();

  InsertOrUpdate(transaction.get(), conversions);

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void Conversions::GetAll(GetConversionsCallback callback) {
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
      get_table_name().c_str(),
      TimeAsTimestampString(base::Time::Now()).c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // type
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // url_pattern
      mojom::DBCommand::RecordBindingType::
          STRING_TYPE,                                 // advertiser_public_key
      mojom::DBCommand::RecordBindingType::INT_TYPE,   // observation_window
      mojom::DBCommand::RecordBindingType::INT64_TYPE  // expiry_timestamp
  };

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction), std::bind(&Conversions::OnGetConversions, this,
                                        std::placeholders::_1, callback));
}

void Conversions::PurgeExpired(ResultCallback callback) {
  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "DELETE FROM %s "
      "WHERE %s >= expiry_timestamp",
      get_table_name().c_str(),
      TimeAsTimestampString(base::Time::Now()).c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

std::string Conversions::get_table_name() const {
  return kTableName;
}

void Conversions::Migrate(mojom::DBTransaction* transaction,
                          const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 1: {
      MigrateToV1(transaction);
      break;
    }

    case 10: {
      MigrateToV10(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void Conversions::InsertOrUpdate(mojom::DBTransaction* transaction,
                                 const ConversionList& conversions) {
  DCHECK(transaction);

  if (conversions.empty()) {
    return;
  }

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), conversions);

  transaction->commands.push_back(std::move(command));
}

int Conversions::BindParameters(mojom::DBCommand* command,
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
    BindInt64(command, index++, conversion.expiry_timestamp);

    count++;
  }

  return count;
}

std::string Conversions::BuildInsertOrUpdateQuery(
    mojom::DBCommand* command,
    const ConversionList& conversions) {
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
      get_table_name().c_str(),
      BuildBindingParameterPlaceholders(6, count).c_str());
}

void Conversions::OnGetConversions(mojom::DBCommandResponsePtr response,
                                   GetConversionsCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get creative conversions");
    callback(/* success */ false, {});
    return;
  }

  ConversionList conversions;

  for (const auto& record : response->result->get_records()) {
    ConversionInfo info = GetConversionFromRecord(record.get());
    conversions.push_back(info);
  }

  callback(/* success */ true, conversions);
}

ConversionInfo Conversions::GetConversionFromRecord(
    mojom::DBRecord* record) const {
  ConversionInfo info;

  info.creative_set_id = ColumnString(record, 0);
  info.type = ColumnString(record, 1);
  info.url_pattern = ColumnString(record, 2);
  info.advertiser_public_key = ColumnString(record, 3);
  info.observation_window = ColumnInt(record, 4);
  info.expiry_timestamp = ColumnInt64(record, 5);

  return info;
}

void Conversions::CreateTableV1(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE ad_conversions "
      "(creative_set_id TEXT NOT NULL, "
      "type TEXT NOT NULL, "
      "url_pattern TEXT NOT NULL, "
      "observation_window INTEGER NOT NULL, "
      "expiry_timestamp TIMESTAMP NOT NULL, "
      "UNIQUE(creative_set_id, type, url_pattern) ON CONFLICT REPLACE, "
      "PRIMARY KEY(creative_set_id, type, url_pattern))");

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void Conversions::CreateIndexV1(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  util::CreateIndex(transaction, "ad_conversions", "creative_set_id");
}

void Conversions::MigrateToV1(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, "ad_conversions");

  CreateTableV1(transaction);
  CreateIndexV1(transaction);
}

void Conversions::MigrateToV10(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  util::Rename(transaction, "ad_conversions", get_table_name());

  const std::string query = base::StringPrintf(
      "ALTER TABLE %s "
      "ADD COLUMN advertiser_public_key TEXT",
      get_table_name().c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  util::CreateIndex(transaction, get_table_name(), "creative_set_id");
}

}  // namespace table
}  // namespace database
}  // namespace ads
