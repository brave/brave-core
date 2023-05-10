/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_events_database_table.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "ad_events";

void BindRecords(mojom::DBCommandInfo* command) {
  CHECK(command);

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // placement_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // type
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // confirmation
                                                             // type
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE   // created_at
  };
}

size_t BindParameters(mojom::DBCommandInfo* command,
                      const AdEventList& ad_events) {
  CHECK(command);

  size_t count = 0;

  int index = 0;
  for (const auto& ad_event : ad_events) {
    BindString(command, index++, ad_event.placement_id);
    BindString(command, index++, ad_event.type.ToString());
    BindString(command, index++, ad_event.confirmation_type.ToString());
    BindString(command, index++, ad_event.campaign_id);
    BindString(command, index++, ad_event.creative_set_id);
    BindString(command, index++, ad_event.creative_instance_id);
    BindString(command, index++, ad_event.advertiser_id);
    BindString(command, index++, ad_event.segment);
    BindDouble(command, index++, ad_event.created_at.ToDoubleT());

    count++;
  }

  return count;
}

AdEventInfo GetFromRecord(mojom::DBRecordInfo* record) {
  CHECK(record);

  AdEventInfo ad_event;

  ad_event.placement_id = ColumnString(record, 0);
  ad_event.type = AdType(ColumnString(record, 1));
  ad_event.confirmation_type = ConfirmationType(ColumnString(record, 2));
  ad_event.campaign_id = ColumnString(record, 3);
  ad_event.creative_set_id = ColumnString(record, 4);
  ad_event.creative_instance_id = ColumnString(record, 5);
  ad_event.advertiser_id = ColumnString(record, 6);
  ad_event.segment = ColumnString(record, 7);
  ad_event.created_at = base::Time::FromDoubleT(ColumnDouble(record, 8));

  return ad_event;
}

void OnGetAdEvents(GetAdEventsCallback callback,
                   mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get ad events");
    std::move(callback).Run(/*success*/ false, /*ad_events*/ {});
    return;
  }

  AdEventList ad_events;

  for (const auto& record : command_response->result->get_records()) {
    const AdEventInfo ad_event = GetFromRecord(&*record);
    ad_events.push_back(ad_event);
  }

  std::move(callback).Run(/*success*/ true, ad_events);
}

void MigrateToV5(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  DropTable(transaction, "ad_events");

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE ad_events (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "uuid TEXT NOT NULL, type TEXT, confirmation_type TEXT, campaign_id TEXT "
      "NOT NULL, creative_set_id TEXT NOT NULL, creative_instance_id TEXT NOT "
      "NULL, advertiser_id TEXT, timestamp TIMESTAMP NOT NULL);";
  transaction->commands.push_back(std::move(command));
}

void MigrateToV13(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  // Create a temporary table with new |advertiser_id| column.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE ad_events_temp (id INTEGER PRIMARY KEY AUTOINCREMENT NOT "
      "NULL, uuid TEXT NOT NULL, type TEXT, confirmation_type TEXT, "
      "campaign_id TEXT NOT NULL, creative_set_id TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, advertiser_id TEXT, segment TEXT, "
      "timestamp TIMESTAMP NOT NULL);";
  transaction->commands.push_back(std::move(command));

  // Copy columns to temporary table.
  const std::vector<std::string> columns = {"uuid",
                                            "type",
                                            "confirmation_type",
                                            "campaign_id",
                                            "creative_set_id",
                                            "creative_instance_id",
                                            "timestamp"};

  CopyTableColumns(transaction, "ad_events", "ad_events_temp", columns,
                   /*should_drop*/ true);

  // Rename temporary table.
  RenameTable(transaction, "ad_events_temp", "ad_events");
}

void MigrateToV17(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  CreateTableIndex(transaction, "ad_events", "timestamp");
}

void MigrateToV28(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  // Create a temporary table with new |segment| column.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE ad_events_temp (id INTEGER PRIMARY KEY AUTOINCREMENT NOT "
      "NULL, placement_id TEXT NOT NULL, type TEXT, confirmation_type TEXT, "
      "campaign_id TEXT NOT NULL, creative_set_id TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, advertiser_id TEXT, segment TEXT, "
      "created_at TIMESTAMP NOT NULL);";
  transaction->commands.push_back(std::move(command));

  // Copy columns to temporary table.
  const std::vector<std::string> from_columns = {"uuid",
                                                 "type",
                                                 "confirmation_type",
                                                 "campaign_id",
                                                 "creative_set_id",
                                                 "creative_instance_id",
                                                 "advertiser_id",
                                                 "timestamp"};

  const std::vector<std::string> to_columns = {
      "placement_id",      "type",
      "confirmation_type", "campaign_id",
      "creative_set_id",   "creative_instance_id",
      "advertiser_id",     "created_at"};

  CopyTableColumns(transaction, "ad_events", "ad_events_temp", from_columns,
                   to_columns,
                   /*should_drop*/ true);

  // Rename temporary table.
  RenameTable(transaction, "ad_events_temp", "ad_events");

  CreateTableIndex(transaction, "ad_events", "created_at");
}

}  // namespace

void AdEvents::LogEvent(const AdEventInfo& ad_event, ResultCallback callback) {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  InsertOrUpdate(&*transaction, {ad_event});

  RunTransaction(std::move(transaction), std::move(callback));
}

void AdEvents::GetIf(const std::string& condition,
                     GetAdEventsCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
      "SELECT ae.placement_id, ae.type, ae.confirmation_type, ae.campaign_id, "
      "ae.creative_set_id, ae.creative_instance_id, ae.advertiser_id, "
      "ae.segment, ae.created_at FROM $1 AS ae WHERE $2 ORDER BY created_at "
      "DESC;",
      {GetTableName(), condition}, nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnGetAdEvents, std::move(callback)));
}

void AdEvents::GetAll(GetAdEventsCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
      "SELECT ae.placement_id, ae.type, ae.confirmation_type, ae.campaign_id, "
      "ae.creative_set_id, ae.creative_instance_id, ae.advertiser_id, "
      "ae.segment, ae.created_at FROM $1 AS ae ORDER BY created_at DESC;",
      {GetTableName()}, nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnGetAdEvents, std::move(callback)));
}

void AdEvents::GetForType(const mojom::AdType ad_type,
                          GetAdEventsCallback callback) const {
  CHECK(mojom::IsKnownEnumValue(ad_type));

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
      "SELECT ae.placement_id, ae.type, ae.confirmation_type, ae.campaign_id, "
      "ae.creative_set_id, ae.creative_instance_id, ae.advertiser_id, "
      "ae.segment, ae.created_at FROM $1 AS ae WHERE type = '$2' ORDER BY "
      "created_at DESC;",
      {GetTableName(), AdType(ad_type).ToString()}, nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnGetAdEvents, std::move(callback)));
}

void AdEvents::PurgeExpired(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      "DELETE FROM $1 WHERE creative_set_id NOT IN (SELECT creative_set_id "
      "from creative_ads) AND creative_set_id NOT IN (SELECT creative_set_id "
      "from creative_ad_conversions) AND DATETIME('now') >= "
      "DATETIME(created_at, 'unixepoch', '+3 month');",
      {GetTableName()}, nullptr);
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

void AdEvents::PurgeOrphaned(const mojom::AdType ad_type,
                             ResultCallback callback) const {
  CHECK(mojom::IsKnownEnumValue(ad_type));

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      "DELETE FROM $1 WHERE placement_id IN (SELECT placement_id from $2 GROUP "
      "BY placement_id having count(*) = 1) AND confirmation_type IN (SELECT "
      "confirmation_type from $3 WHERE confirmation_type = 'served') AND type "
      "= '$4';",
      {GetTableName(), GetTableName(), GetTableName(),
       AdType(ad_type).ToString()},
      nullptr);
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

std::string AdEvents::GetTableName() const {
  return kTableName;
}

void AdEvents::Create(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE ad_events (id INTEGER PRIMARY KEY "
      "AUTOINCREMENT NOT NULL, placement_id TEXT NOT NULL, type TEXT, "
      "confirmation_type TEXT, campaign_id TEXT NOT NULL, creative_set_id TEXT "
      "NOT NULL, creative_instance_id TEXT NOT NULL, advertiser_id TEXT, "
      "segment TEXT, created_at TIMESTAMP NOT NULL);";
  transaction->commands.push_back(std::move(command));
}

void AdEvents::Migrate(mojom::DBTransactionInfo* transaction,
                       const int to_version) {
  CHECK(transaction);

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

void AdEvents::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                              const AdEventList& ad_events) {
  CHECK(transaction);

  if (ad_events.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = BuildInsertOrUpdateSql(&*command, ad_events);
  transaction->commands.push_back(std::move(command));
}

std::string AdEvents::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const AdEventList& ad_events) const {
  CHECK(command);

  const size_t binded_parameters_count = BindParameters(command, ad_events);

  return base::ReplaceStringPlaceholders(
      "INSERT OR REPLACE INTO $1 (placement_id, type, confirmation_type, "
      "campaign_id, creative_set_id, creative_instance_id, advertiser_id, "
      "segment, created_at) VALUES $2;",
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count*/ 9, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
