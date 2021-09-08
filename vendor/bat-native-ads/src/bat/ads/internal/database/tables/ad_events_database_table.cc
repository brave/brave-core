/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/ad_events_database_table.h"

#include <cstdint>
#include <functional>
#include <utility>

#include "base/check.h"
#include "base/strings/stringprintf.h"
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
  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();

  InsertOrUpdate(transaction.get(), {ad_event});

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void AdEvents::GetIf(const std::string& condition,
                     GetAdEventsCallback callback) {
  const std::string query = base::StringPrintf(
      "SELECT "
      "ae.uuid, "
      "ae.type, "
      "ae.confirmation_type, "
      "ae.campaign_id, "
      "ae.creative_set_id, "
      "ae.creative_instance_id, "
      "ae.advertiser_id, "
      "ae.timestamp "
      "FROM %s AS ae "
      "WHERE %s "
      "ORDER BY timestamp DESC ",
      get_table_name().c_str(), condition.c_str());

  RunTransaction(query, callback);
}

void AdEvents::GetAll(GetAdEventsCallback callback) {
  const std::string query = base::StringPrintf(
      "SELECT "
      "ae.uuid, "
      "ae.type, "
      "ae.confirmation_type, "
      "ae.campaign_id, "
      "ae.creative_set_id, "
      "ae.creative_instance_id, "
      "ae.advertiser_id, "
      "ae.timestamp "
      "FROM %s AS ae "
      "ORDER BY timestamp DESC",
      get_table_name().c_str());

  RunTransaction(query, callback);
}

void AdEvents::PurgeExpired(ResultCallback callback) {
  const std::string query = base::StringPrintf(
      "DELETE FROM %s "
      "WHERE creative_set_id NOT IN "
      "(SELECT creative_set_id from creative_ads) "
      "AND creative_set_id NOT IN "
      "(SELECT creative_set_id from creative_ad_conversions) "
      "AND DATETIME('now') >= DATETIME(timestamp, 'unixepoch', '+3 month')",
      get_table_name().c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void AdEvents::PurgeOrphaned(const mojom::AdType ad_type,
                             ResultCallback callback) {
  const std::string ad_type_as_string = AdType(ad_type).ToString();

  const std::string query = base::StringPrintf(
      "DELETE FROM %s "
      "WHERE uuid IN (SELECT uuid from %s GROUP BY uuid having count(*) = 1) "
      "AND confirmation_type IN (SELECT confirmation_type from %s "
      "WHERE confirmation_type = 'served') "
      "AND type = '%s'",
      get_table_name().c_str(), get_table_name().c_str(),
      get_table_name().c_str(), ad_type_as_string.c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

std::string AdEvents::get_table_name() const {
  return kTableName;
}

void AdEvents::Migrate(mojom::DBTransaction* transaction,
                       const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 5: {
      MigrateToV5(transaction);
      break;
    }

    case 13: {
      MigrateToV13(transaction);
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
  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // uuid
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // type
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // confirmation type
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_instance_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommand::RecordBindingType::INT64_TYPE    // timestamp
  };

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction), std::bind(&AdEvents::OnGetAdEvents, this,
                                        std::placeholders::_1, callback));
}

void AdEvents::InsertOrUpdate(mojom::DBTransaction* transaction,
                              const AdEventList& ad_events) {
  DCHECK(transaction);

  if (ad_events.empty()) {
    return;
  }

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), ad_events);

  transaction->commands.push_back(std::move(command));
}

int AdEvents::BindParameters(mojom::DBCommand* command,
                             const AdEventList& ad_events) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& ad_event : ad_events) {
    BindString(command, index++, ad_event.uuid);
    BindString(command, index++, ad_event.type);
    BindString(command, index++, ad_event.confirmation_type);
    BindString(command, index++, ad_event.campaign_id);
    BindString(command, index++, ad_event.creative_set_id);
    BindString(command, index++, ad_event.creative_instance_id);
    BindString(command, index++, ad_event.advertiser_id);
    BindInt64(command, index++, ad_event.timestamp);

    count++;
  }

  return count;
}

std::string AdEvents::BuildInsertOrUpdateQuery(mojom::DBCommand* command,
                                               const AdEventList& ad_events) {
  DCHECK(command);

  const int count = BindParameters(command, ad_events);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(uuid, "
      "type, "
      "confirmation_type, "
      "campaign_id, "
      "creative_set_id, "
      "creative_instance_id, "
      "advertiser_id, "
      "timestamp) VALUES %s",
      get_table_name().c_str(),
      BuildBindingParameterPlaceholders(8, count).c_str());
}

void AdEvents::OnGetAdEvents(mojom::DBCommandResponsePtr response,
                             GetAdEventsCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get ad events");
    callback(/* success */ false, {});
    return;
  }

  AdEventList ad_events;

  for (const auto& record : response->result->get_records()) {
    AdEventInfo info = GetFromRecord(record.get());
    ad_events.push_back(info);
  }

  callback(/* success */ true, ad_events);
}

AdEventInfo AdEvents::GetFromRecord(mojom::DBRecord* record) const {
  AdEventInfo info;

  info.uuid = ColumnString(record, 0);
  info.type = AdType(ColumnString(record, 1));
  info.confirmation_type = ConfirmationType(ColumnString(record, 2));
  info.campaign_id = ColumnString(record, 3);
  info.creative_set_id = ColumnString(record, 4);
  info.creative_instance_id = ColumnString(record, 5);
  info.advertiser_id = ColumnString(record, 6);
  info.timestamp = ColumnInt64(record, 7);

  return info;
}

void AdEvents::CreateTableV5(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE ad_events "
      "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "uuid TEXT NOT NULL, "
      "type TEXT, "
      "confirmation_type TEXT, "
      "campaign_id TEXT NOT NULL, "
      "creative_set_id TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, "
      "advertiser_id TEXT, "
      "timestamp TIMESTAMP NOT NULL)");

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void AdEvents::MigrateToV5(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, "ad_events");

  CreateTableV5(transaction);
}

void AdEvents::CreateTableV13(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string query = base::StringPrintf(
      "CREATE TABLE ad_events "
      "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "uuid TEXT NOT NULL, "
      "type TEXT, "
      "confirmation_type TEXT, "
      "campaign_id TEXT NOT NULL, "
      "creative_set_id TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, "
      "advertiser_id TEXT, "
      "timestamp TIMESTAMP NOT NULL)");

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void AdEvents::MigrateToV13(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  util::Rename(transaction, "ad_events", "ad_events_temp");

  CreateTableV13(transaction);

  const std::string query = base::StringPrintf(
      "INSERT INTO ad_events "
      "(id, "
      "uuid, "
      "type, "
      "confirmation_type, "
      "campaign_id, "
      "creative_set_id, "
      "creative_instance_id, "
      "timestamp) "
      "SELECT id, "
      "uuid, "
      "type, "
      "confirmation_type, "
      "campaign_id, "
      "creative_set_id, "
      "creative_instance_id, "
      "timestamp "
      "FROM ad_events_temp");

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  util::Drop(transaction, "ad_events_temp");
}

}  // namespace table
}  // namespace database
}  // namespace ads
