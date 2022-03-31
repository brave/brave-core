/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/ad_events_database_table.h"

#include <utility>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace database {
namespace table {

namespace {

constexpr char kTableName[] = "ad_events";

int BindParameters(mojom::DBCommand* command, const AdEventList& ad_events) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& ad_event : ad_events) {
    BindString(command, index++, ad_event.uuid);
    BindString(command, index++, ad_event.type.ToString());
    BindString(command, index++, ad_event.confirmation_type.ToString());
    BindString(command, index++, ad_event.campaign_id);
    BindString(command, index++, ad_event.creative_set_id);
    BindString(command, index++, ad_event.creative_instance_id);
    BindString(command, index++, ad_event.advertiser_id);
    BindDouble(command, index++, ad_event.created_at.ToDoubleT());

    count++;
  }

  return count;
}

AdEventInfo GetFromRecord(mojom::DBRecord* record) {
  DCHECK(record);

  AdEventInfo ad_event;

  ad_event.uuid = ColumnString(record, 0);
  ad_event.type = AdType(ColumnString(record, 1));
  ad_event.confirmation_type = ConfirmationType(ColumnString(record, 2));
  ad_event.campaign_id = ColumnString(record, 3);
  ad_event.creative_set_id = ColumnString(record, 4);
  ad_event.creative_instance_id = ColumnString(record, 5);
  ad_event.advertiser_id = ColumnString(record, 6);
  ad_event.created_at = base::Time::FromDoubleT(ColumnDouble(record, 7));

  return ad_event;
}

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
  const std::string& query = base::StringPrintf(
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
      GetTableName().c_str(), condition.c_str());

  RunTransaction(query, callback);
}

void AdEvents::GetAll(GetAdEventsCallback callback) {
  const std::string& query = base::StringPrintf(
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
      GetTableName().c_str());

  RunTransaction(query, callback);
}

void AdEvents::GetForType(const mojom::AdType ad_type,
                          GetAdEventsCallback callback) {
  const std::string& ad_type_as_string = AdType(ad_type).ToString();

  const std::string& query = base::StringPrintf(
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
      "WHERE type = '%s' "
      "ORDER BY timestamp DESC",
      GetTableName().c_str(), ad_type_as_string.c_str());

  RunTransaction(query, callback);
}

void AdEvents::PurgeExpired(ResultCallback callback) {
  const std::string& query = base::StringPrintf(
      "DELETE FROM %s "
      "WHERE creative_set_id NOT IN "
      "(SELECT creative_set_id from creative_ads) "
      "AND creative_set_id NOT IN "
      "(SELECT creative_set_id from creative_ad_conversions) "
      "AND DATETIME('now') >= DATETIME(timestamp, 'unixepoch', '+3 month')",
      GetTableName().c_str());

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
  const std::string& ad_type_as_string = AdType(ad_type).ToString();

  const std::string& query = base::StringPrintf(
      "DELETE FROM %s "
      "WHERE uuid IN (SELECT uuid from %s GROUP BY uuid having count(*) = 1) "
      "AND confirmation_type IN (SELECT confirmation_type from %s "
      "WHERE confirmation_type = 'served') "
      "AND type = '%s'",
      GetTableName().c_str(), GetTableName().c_str(), GetTableName().c_str(),
      ad_type_as_string.c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

std::string AdEvents::GetTableName() const {
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

    case 17: {
      MigrateToV17(transaction);
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
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE   // created_at
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
      GetTableName().c_str(),
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
    const AdEventInfo& ad_event = GetFromRecord(record.get());
    ad_events.push_back(ad_event);
  }

  callback(/* success */ true, ad_events);
}

void AdEvents::MigrateToV5(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, "ad_events");

  const std::string& query = base::StringPrintf(
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

  const std::string& query = base::StringPrintf(
      "CREATE TABLE ad_events "
      "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "uuid TEXT NOT NULL, "
      "type TEXT, "
      "confirmation_type TEXT, "
      "campaign_id TEXT NOT NULL, "
      "creative_set_id TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, "
      "advertiser_id TEXT, "
      "timestamp TIMESTAMP NOT NULL); "
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

void AdEvents::MigrateToV17(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  util::CreateIndex(transaction, "ad_events", "timestamp");
}

}  // namespace table
}  // namespace database
}  // namespace ads
