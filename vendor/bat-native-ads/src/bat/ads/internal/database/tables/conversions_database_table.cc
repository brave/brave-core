/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/conversions_database_table.h"

#include <functional>
#include <utility>

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
    callback(Result::SUCCESS);
    return;
  }

  DBTransactionPtr transaction = DBTransaction::New();

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

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      DBCommand::RecordBindingType::STRING_TYPE,  // creative_set_id
      DBCommand::RecordBindingType::STRING_TYPE,  // type
      DBCommand::RecordBindingType::STRING_TYPE,  // url_pattern
      DBCommand::RecordBindingType::STRING_TYPE,  // advertiser_public_key
      DBCommand::RecordBindingType::INT_TYPE,     // observation_window
      DBCommand::RecordBindingType::INT64_TYPE    // expiry_timestamp
  };

  DBTransactionPtr transaction = DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction), std::bind(&Conversions::OnGetConversions, this,
                                        std::placeholders::_1, callback));
}

void Conversions::PurgeExpired(ResultCallback callback) {
  DBTransactionPtr transaction = DBTransaction::New();

  const std::string query = base::StringPrintf(
      "DELETE FROM %s "
      "WHERE %s >= expiry_timestamp",
      get_table_name().c_str(),
      TimeAsTimestampString(base::Time::Now()).c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

std::string Conversions::get_table_name() const {
  return kTableName;
}

void Conversions::Migrate(DBTransaction* transaction, const int to_version) {
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

void Conversions::InsertOrUpdate(DBTransaction* transaction,
                                 const ConversionList& conversions) {
  DCHECK(transaction);

  if (conversions.empty()) {
    return;
  }

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), conversions);

  transaction->commands.push_back(std::move(command));
}

int Conversions::BindParameters(DBCommand* command,
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
    DBCommand* command,
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

void Conversions::OnGetConversions(DBCommandResponsePtr response,
                                   GetConversionsCallback callback) {
  if (!response || response->status != DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get creative conversions");
    callback(Result::FAILED, {});
    return;
  }

  ConversionList conversions;

  for (const auto& record : response->result->get_records()) {
    ConversionInfo info = GetConversionFromRecord(record.get());
    conversions.push_back(info);
  }

  callback(Result::SUCCESS, conversions);
}

ConversionInfo Conversions::GetConversionFromRecord(DBRecord* record) const {
  ConversionInfo info;

  info.creative_set_id = ColumnString(record, 0);
  info.type = ColumnString(record, 1);
  info.url_pattern = ColumnString(record, 2);
  info.advertiser_public_key = ColumnString(record, 3);
  info.observation_window = ColumnInt(record, 4);
  info.expiry_timestamp = ColumnInt64(record, 5);

  return info;
}

void Conversions::CreateTableV1(DBTransaction* transaction) {
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

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void Conversions::CreateIndexV1(DBTransaction* transaction) {
  DCHECK(transaction);

  util::CreateIndex(transaction, "ad_conversions", "creative_set_id");
}

void Conversions::MigrateToV1(DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, "ad_conversions");

  CreateTableV1(transaction);
  CreateIndexV1(transaction);
}

void Conversions::MigrateToV10(DBTransaction* transaction) {
  DCHECK(transaction);

  util::Rename(transaction, "ad_conversions", get_table_name());

  const std::string query = base::StringPrintf(
      "ALTER TABLE %s "
      "ADD COLUMN advertiser_public_key TEXT",
      get_table_name().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  util::CreateIndex(transaction, get_table_name(), "creative_set_id");
}

}  // namespace table
}  // namespace database
}  // namespace ads
