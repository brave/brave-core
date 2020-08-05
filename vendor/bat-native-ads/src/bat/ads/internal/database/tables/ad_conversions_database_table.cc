/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/ad_conversions_database_table.h"

#include <functional>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time_util.h"

namespace ads {
namespace database {
namespace table {

using std::placeholders::_1;

namespace {
const char kTableName[] = "ad_conversions";
}  // namespace

AdConversions::AdConversions(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdConversions::~AdConversions() = default;

void AdConversions::Save(
    const AdConversionList& ad_conversions,
    ResultCallback callback) {
  if (ad_conversions.empty()) {
    callback(Result::SUCCESS);
    return;
  }

  DBTransactionPtr transaction = DBTransaction::New();

  InsertOrUpdate(transaction.get(), ad_conversions);

  ads_->get_ads_client()->RunDBTransaction(std::move(transaction),
      std::bind(&OnResultCallback, _1, callback));
}

void AdConversions::GetAdConversions(
    GetAdConversionsCallback callback) {
  const std::string query = base::StringPrintf(
      "SELECT "
          "ac.creative_set_id, "
          "ac.type, "
          "ac.url_pattern, "
          "ac.observation_window, "
          "ac.expiry_timestamp "
      "FROM %s AS ac "
      "WHERE %s < expiry_timestamp",
      get_table_name().c_str(),
      NowAsString().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
    DBCommand::RecordBindingType::STRING_TYPE,  // creative_set_id
    DBCommand::RecordBindingType::STRING_TYPE,  // type
    DBCommand::RecordBindingType::STRING_TYPE,  // url_pattern
    DBCommand::RecordBindingType::INT_TYPE,     // observation_window
    DBCommand::RecordBindingType::INT64_TYPE    // expiry_timestamp
  };

  DBTransactionPtr transaction = DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ads_->get_ads_client()->RunDBTransaction(std::move(transaction),
      std::bind(&AdConversions::OnGetAdConversions, this, _1, callback));
}

void AdConversions::PurgeExpiredAdConversions(
    ResultCallback callback) {
  DBTransactionPtr transaction = DBTransaction::New();

  const std::string query = base::StringPrintf(
      "DELETE FROM %s "
      "WHERE %s >= expiry_timestamp",
      get_table_name().c_str(),
      NowAsString().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  ads_->get_ads_client()->RunDBTransaction(std::move(transaction),
      std::bind(&OnResultCallback, _1, callback));
}

std::string AdConversions::get_table_name() const {
  return kTableName;
}

void AdConversions::Migrate(
    DBTransaction* transaction,
    const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 1: {
      MigrateToV1(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void AdConversions::InsertOrUpdate(
    DBTransaction* transaction,
    const AdConversionList& ad_conversions) {
  DCHECK(transaction);

  if (ad_conversions.empty()) {
    return;
  }

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), ad_conversions);

  transaction->commands.push_back(std::move(command));
}

int AdConversions::BindParameters(
    DBCommand* command,
    const AdConversionList& ad_conversions) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& ad_conversion : ad_conversions) {
    BindString(command, index++, ad_conversion.creative_set_id);
    BindString(command, index++, ad_conversion.type);
    BindString(command, index++, ad_conversion.url_pattern);
    BindInt(command, index++, ad_conversion.observation_window);
    BindInt64(command, index++, ad_conversion.expiry_timestamp);

    count++;
  }

  return count;
}

std::string AdConversions::BuildInsertOrUpdateQuery(
    DBCommand* command,
    const AdConversionList& ad_conversions) {
  DCHECK(command);

  const int count = BindParameters(command, ad_conversions);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
          "(creative_set_id, "
          "type, "
          "url_pattern, "
          "observation_window, "
          "expiry_timestamp) VALUES %s",
    get_table_name().c_str(),
    BuildBindingParameterPlaceholders(5, count).c_str());
}

void AdConversions::OnGetAdConversions(
    DBCommandResponsePtr response,
    GetAdConversionsCallback callback) {
  if (!response || response->status != DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get creative ad conversions");
    callback(Result::FAILED, {});
    return;
  }

  AdConversionList ad_conversions;

  for (auto const& record : response->result->get_records()) {
    AdConversionInfo info = GetAdConversionFromRecord(record.get());

    ad_conversions.emplace_back(info);
  }

  callback(Result::SUCCESS, ad_conversions);
}

AdConversionInfo AdConversions::GetAdConversionFromRecord(
    DBRecord* record) const {
  AdConversionInfo info;

  info.creative_set_id = ColumnString(record, 0);
  info.type = ColumnString(record, 1);
  info.url_pattern = ColumnString(record, 2);
  info.observation_window = ColumnInt(record, 3);
  info.expiry_timestamp = ColumnInt64(record, 4);

  return info;
}

void AdConversions::CreateTableV1(
    DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s "
          "(creative_set_id TEXT NOT NULL, "
          "type TEXT NOT NULL, "
          "url_pattern TEXT NOT NULL, "
          "observation_window INTEGER NOT NULL, "
          "expiry_timestamp TIMESTAMP NOT NULL, "
          "UNIQUE(creative_set_id, type, url_pattern) ON CONFLICT REPLACE, "
          "PRIMARY KEY(creative_set_id, type, url_pattern))",
      get_table_name().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void AdConversions::CreateIndexV1(
    DBTransaction* transaction) {
  DCHECK(transaction);

  CreateIndex(transaction, get_table_name(), "creative_set_id");
}

void AdConversions::MigrateToV1(
    DBTransaction* transaction) {
  DCHECK(transaction);

  Drop(transaction, get_table_name());

  CreateTableV1(transaction);
  CreateIndexV1(transaction);
}

}  // namespace table
}  // namespace database
}  // namespace ads
