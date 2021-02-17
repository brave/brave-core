/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/ad_events_database_table.h"

#include <functional>
#include <utility>

#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace database {
namespace table {

namespace {
const char kTableName[] = "ad_events";
}  // namespace

AdEvents::AdEvents() = default;

AdEvents::~AdEvents() = default;

void AdEvents::LogEvent(const AdEventInfo& ad_event, ResultCallback callback) {
  DBTransactionPtr transaction = DBTransaction::New();

  InsertOrUpdate(transaction.get(), {ad_event});

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void AdEvents::GetIf(const std::string& condition,
                     GetAdEventsCallback callback) {
  const std::string query = base::StringPrintf(
      "SELECT "
      "ae.type, "
      "ae.uuid, "
      "ae.creative_instance_id, "
      "ae.creative_set_id, "
      "ae.campaign_id, "
      "ae.timestamp, "
      "ae.confirmation_type "
      "FROM %s AS ae "
      "WHERE %s "
      "ORDER BY timestamp DESC ",
      get_table_name().c_str(), condition.c_str());

  RunTransaction(query, callback);
}

void AdEvents::GetAll(GetAdEventsCallback callback) {
  const std::string query = base::StringPrintf(
      "SELECT "
      "ae.type, "
      "ae.uuid, "
      "ae.creative_instance_id, "
      "ae.creative_set_id, "
      "ae.campaign_id, "
      "ae.timestamp, "
      "ae.confirmation_type "
      "FROM %s AS ae "
      "ORDER BY timestamp DESC",
      get_table_name().c_str());

  RunTransaction(query, callback);
}

void AdEvents::PurgeExpired(ResultCallback callback) {
  DBTransactionPtr transaction = DBTransaction::New();

  const std::string query = base::StringPrintf(
      "DELETE FROM %s "
      "WHERE creative_set_id NOT IN "
      "(SELECT creative_set_id from creative_ads) "
      "AND creative_set_id NOT IN "
      "(SELECT creative_set_id from ad_conversions) "
      "AND DATETIME('now') >= DATETIME(timestamp, 'unixepoch', '+3 month')",
      get_table_name().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

std::string AdEvents::get_table_name() const {
  return kTableName;
}

void AdEvents::Migrate(DBTransaction* transaction, const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 5: {
      MigrateToV5(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void AdEvents::RunTransaction(const std::string& query,
                              GetAdEventsCallback callback) {
  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      DBCommand::RecordBindingType::STRING_TYPE,  // type
      DBCommand::RecordBindingType::STRING_TYPE,  // uuid
      DBCommand::RecordBindingType::STRING_TYPE,  // creative_instance_id
      DBCommand::RecordBindingType::STRING_TYPE,  // creative_set_id
      DBCommand::RecordBindingType::STRING_TYPE,  // campaign_id
      DBCommand::RecordBindingType::INT64_TYPE,   // timestamp
      DBCommand::RecordBindingType::STRING_TYPE   // confirmation type
  };

  DBTransactionPtr transaction = DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction), std::bind(&AdEvents::OnGetAdEvents, this,
                                        std::placeholders::_1, callback));
}

void AdEvents::InsertOrUpdate(DBTransaction* transaction,
                              const AdEventList& ad_events) {
  DCHECK(transaction);

  if (ad_events.empty()) {
    return;
  }

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), ad_events);

  transaction->commands.push_back(std::move(command));
}

int AdEvents::BindParameters(DBCommand* command, const AdEventList& ad_events) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& ad_event : ad_events) {
    BindString(command, index++, ad_event.type);
    BindString(command, index++, ad_event.uuid);
    BindString(command, index++, ad_event.creative_instance_id);
    BindString(command, index++, ad_event.creative_set_id);
    BindString(command, index++, ad_event.campaign_id);
    BindInt64(command, index++, ad_event.timestamp);
    BindString(command, index++, ad_event.confirmation_type);

    count++;
  }

  return count;
}

std::string AdEvents::BuildInsertOrUpdateQuery(DBCommand* command,
                                               const AdEventList& ad_events) {
  DCHECK(command);

  const int count = BindParameters(command, ad_events);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(type, "
      "uuid, "
      "creative_instance_id, "
      "creative_set_id, "
      "campaign_id, "
      "timestamp, "
      "confirmation_type) VALUES %s",
      get_table_name().c_str(),
      BuildBindingParameterPlaceholders(7, count).c_str());
}

void AdEvents::OnGetAdEvents(DBCommandResponsePtr response,
                             GetAdEventsCallback callback) {
  if (!response || response->status != DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get ad events");
    callback(Result::FAILED, {});
    return;
  }

  AdEventList ad_events;

  for (const auto& record : response->result->get_records()) {
    AdEventInfo info = GetFromRecord(record.get());
    ad_events.push_back(info);
  }

  callback(Result::SUCCESS, ad_events);
}

AdEventInfo AdEvents::GetFromRecord(DBRecord* record) const {
  AdEventInfo info;

  info.type = AdType(ColumnString(record, 0));
  info.uuid = ColumnString(record, 1);
  info.creative_instance_id = ColumnString(record, 2);
  info.creative_set_id = ColumnString(record, 3);
  info.campaign_id = ColumnString(record, 4);
  info.timestamp = ColumnInt64(record, 5);
  info.confirmation_type = ConfirmationType(ColumnString(record, 6));

  return info;
}

void AdEvents::CreateTableV5(DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE %s "
      "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "type TEXT, "
      "uuid TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, "
      "creative_set_id TEXT NOT NULL, "
      "campaign_id TEXT NOT NULL, "
      "timestamp TIMESTAMP NOT NULL, "
      "confirmation_type TEXT)",
      get_table_name().c_str());

  DBCommandPtr command = DBCommand::New();
  command->type = DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void AdEvents::MigrateToV5(DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, get_table_name());

  CreateTableV5(transaction);
}

}  // namespace table
}  // namespace database
}  // namespace ads
