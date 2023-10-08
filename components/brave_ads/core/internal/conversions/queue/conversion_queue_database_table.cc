/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/queue/conversion_queue_database_table.h"

#include <utility>
#include <vector>

#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/containers/container_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/conversions/actions/conversion_action_types.h"
#include "brave/components/brave_ads/core/internal/conversions/actions/conversion_action_types_constants.h"
#include "brave/components/brave_ads/core/internal/conversions/actions/conversion_action_types_util.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_validation_util.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "conversion_queue";

constexpr int kDefaultBatchSize = 50;

void BindRecords(mojom::DBCommandInfo* command) {
  CHECK(command);

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // ad_type
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // segment
      mojom::DBCommandInfo::RecordBindingType::STRING_TYPE,  // type
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // verifiable_conversion_id
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // verifiable_advertiser_public_key
      mojom::DBCommandInfo::RecordBindingType::INT64_TYPE,  // process_at
      mojom::DBCommandInfo::RecordBindingType::INT_TYPE     // was_processed
  };
}

size_t BindParameters(mojom::DBCommandInfo* command,
                      const ConversionQueueItemList& conversion_queue_items) {
  CHECK(command);

  size_t count = 0;

  int index = 0;
  for (const auto& conversion_queue_item : conversion_queue_items) {
    BindString(command, index++,
               conversion_queue_item.conversion.ad_type.ToString());
    BindString(command, index++, conversion_queue_item.conversion.campaign_id);
    BindString(command, index++,
               conversion_queue_item.conversion.creative_set_id);
    BindString(command, index++,
               conversion_queue_item.conversion.creative_instance_id);
    BindString(command, index++,
               conversion_queue_item.conversion.advertiser_id);
    BindString(command, index++, conversion_queue_item.conversion.segment);
    BindString(command, index++,
               ConversionActionTypeToString(
                   conversion_queue_item.conversion.action_type));
    BindString(command, index++,
               conversion_queue_item.conversion.verifiable
                   ? conversion_queue_item.conversion.verifiable->id
                   : "");
    BindString(command, index++,
               conversion_queue_item.conversion.verifiable
                   ? conversion_queue_item.conversion.verifiable
                         ->advertiser_public_key_base64
                   : "");
    BindInt64(command, index++,
              conversion_queue_item.process_at.ToDeltaSinceWindowsEpoch()
                  .InMicroseconds());
    BindBool(command, index++, conversion_queue_item.was_processed);

    count++;
  }

  return count;
}

ConversionQueueItemInfo GetFromRecord(mojom::DBRecordInfo* record) {
  CHECK(record);

  ConversionQueueItemInfo conversion_queue_item;
  conversion_queue_item.conversion.ad_type = AdType(ColumnString(record, 0));
  conversion_queue_item.conversion.campaign_id = ColumnString(record, 1);
  conversion_queue_item.conversion.creative_set_id = ColumnString(record, 2);
  conversion_queue_item.conversion.creative_instance_id =
      ColumnString(record, 3);
  conversion_queue_item.conversion.advertiser_id = ColumnString(record, 4);
  conversion_queue_item.conversion.segment = ColumnString(record, 5);
  conversion_queue_item.conversion.action_type =
      StringToConversionActionType(ColumnString(record, 6));

  VerifiableConversionInfo verifiable_conversion;
  verifiable_conversion.id = ColumnString(record, 7);
  verifiable_conversion.advertiser_public_key_base64 = ColumnString(record, 8);
  if (verifiable_conversion.IsValid()) {
    conversion_queue_item.conversion.verifiable = verifiable_conversion;
  }

  conversion_queue_item.process_at = base::Time::FromDeltaSinceWindowsEpoch(
      base::Microseconds(ColumnInt64(record, 9)));
  conversion_queue_item.was_processed =
      static_cast<bool>(ColumnInt(record, 10));

  return conversion_queue_item;
}

void GetCallback(GetConversionQueueCallback callback,
                 mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get conversion queue");
    return std::move(callback).Run(/*success=*/false,
                                   /*conversions_queue_items=*/{});
  }

  CHECK(command_response->result);

  ConversionQueueItemList conversion_queue_items;

  for (const auto& record : command_response->result->get_records()) {
    const ConversionQueueItemInfo conversion_queue_item =
        GetFromRecord(&*record);
    // TODO(https://github.com/brave/brave-browser/issues/33239): Validate all
    // Brave Ads data when loading from database
    if (!conversion_queue_item.IsValid()) {
      SCOPED_CRASH_KEY_STRING256(
          "BraveAdsConversion", "invalidFieldsNames",
          GetConversionQueueItemInvalidFieldsNames(conversion_queue_item));
      base::debug::DumpWithoutCrashing();
      continue;
    }

    conversion_queue_items.push_back(conversion_queue_item);
  }

  std::move(callback).Run(/*success=*/true, conversion_queue_items);
}

void GetForCreativeInstanceIdCallback(
    const std::string& creative_instance_id,
    GetConversionQueueForCreativeInstanceIdCallback callback,
    mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get conversion queue");
    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*conversion_queue_items=*/{});
  }

  CHECK(command_response->result);

  ConversionQueueItemList conversion_queue_items;

  for (const auto& record : command_response->result->get_records()) {
    const ConversionQueueItemInfo conversion_queue_item =
        GetFromRecord(&*record);
    conversion_queue_items.push_back(conversion_queue_item);
  }

  std::move(callback).Run(/*success=*/true, creative_instance_id,
                          conversion_queue_items);
}

void MigrateToV10(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  DropTable(transaction, "conversion_queue");

  // campaign_id and advertiser_id can be NULL for legacy conversions migrated
  // from |ad_conversions.json| and conversion_id and advertiser_public_key will
  // be empty for non verifiable conversions
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE conversion_queue (id INTEGER PRIMARY KEY AUTOINCREMENT NOT "
      "NULL, campaign_id TEXT, creative_set_id TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, advertiser_id TEXT, conversion_id "
      "TEXT, timestamp TIMESTAMP NOT NULL);";
  transaction->commands.push_back(std::move(command));
}

void MigrateToV11(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  // Create a temporary table with new |advertiser_public_key| column
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE conversion_queue_temp (id INTEGER PRIMARY KEY "
      "AUTOINCREMENT NOT NULL, campaign_id TEXT, creative_set_id "
      "TEXT NOT NULL, creative_instance_id TEXT NOT NULL, "
      "advertiser_id TEXT, conversion_id TEXT, "
      "advertiser_public_key TEXT, timestamp TIMESTAMP NOT NULL);";
  transaction->commands.push_back(std::move(command));

  // Copy columns to temporary table
  const std::vector<std::string> columns = {
      "campaign_id",   "creative_set_id", "creative_instance_id",
      "advertiser_id", "conversion_id",   "timestamp"};

  CopyTableColumns(transaction, "conversion_queue", "conversion_queue_temp",
                   columns, /*should_drop=*/true);

  // Rename temporary table
  RenameTable(transaction, "conversion_queue_temp", "conversion_queue");
}

void MigrateToV17(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  CreateTableIndex(transaction, "conversion_queue", "creative_instance_id");
}

void MigrateToV21(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  // Create a temporary table with new |ad_type| and |was_processed| column
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE conversion_queue_temp (id INTEGER PRIMARY KEY "
      "AUTOINCREMENT NOT NULL, ad_type TEXT, campaign_id TEXT, creative_set_id "
      "TEXT NOT NULL, creative_instance_id TEXT NOT NULL, advertiser_id TEXT, "
      "conversion_id TEXT, advertiser_public_key TEXT, timestamp TIMESTAMP NOT "
      "NULL, was_processed INTEGER DEFAULT 0);";
  transaction->commands.push_back(std::move(command));

  // Copy columns to temporary table
  const std::vector<std::string> columns = {
      "campaign_id",   "creative_set_id", "creative_instance_id",
      "advertiser_id", "conversion_id",   "advertiser_public_key",
      "timestamp"};

  CopyTableColumns(transaction, "conversion_queue", "conversion_queue_temp",
                   columns, /*should_drop=*/true);

  // Rename temporary table
  RenameTable(transaction, "conversion_queue_temp", "conversion_queue");

  // Migrate legacy conversions
  mojom::DBCommandInfoPtr update_command = mojom::DBCommandInfo::New();
  update_command->type = mojom::DBCommandInfo::Type::EXECUTE;
  update_command->sql =
      "UPDATE conversion_queue SET ad_type = 'ad_notification' WHERE ad_type "
      "IS NULL;";
  transaction->commands.push_back(std::move(update_command));
}

void MigrateToV26(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  // Create a temporary table with new |segment| column
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE conversion_queue_temp (id INTEGER PRIMARY KEY "
      "AUTOINCREMENT NOT NULL, ad_type TEXT, campaign_id TEXT, creative_set_id "
      "TEXT NOT NULL, creative_instance_id TEXT NOT NULL, advertiser_id TEXT, "
      "segment TEXT, conversion_id TEXT, advertiser_public_key TEXT, timestamp "
      "TIMESTAMP NOT NULL, was_processed INTEGER DEFAULT 0);";
  transaction->commands.push_back(std::move(command));

  // Copy columns to temporary table
  const std::vector<std::string> columns = {"ad_type",
                                            "campaign_id",
                                            "creative_set_id",
                                            "creative_instance_id",
                                            "advertiser_id",
                                            "conversion_id",
                                            "advertiser_public_key",
                                            "timestamp",
                                            "was_processed"};

  CopyTableColumns(transaction, "conversion_queue", "conversion_queue_temp",
                   columns,
                   /*should_drop=*/true);

  // Rename temporary table
  RenameTable(transaction, "conversion_queue_temp", "conversion_queue");
}

void MigrateToV28(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  // Create a temporary table with renamed |timestamp| to |process_at| column
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE conversion_queue_temp (id INTEGER PRIMARY KEY "
      "AUTOINCREMENT NOT NULL, ad_type TEXT, campaign_id TEXT, creative_set_id "
      "TEXT NOT NULL, creative_instance_id TEXT NOT NULL, advertiser_id TEXT, "
      "segment TEXT, conversion_id TEXT, advertiser_public_key TEXT, "
      "process_at TIMESTAMP NOT NULL, was_processed INTEGER DEFAULT 0);";
  transaction->commands.push_back(std::move(command));

  // Copy columns to temporary table
  const std::vector<std::string> from_columns = {
      "ad_type",         "campaign_id",
      "creative_set_id", "creative_instance_id",
      "advertiser_id",   "segment",
      "conversion_id",   "advertiser_public_key",
      "timestamp",       "was_processed"};

  const std::vector<std::string> to_columns = {
      "ad_type",         "campaign_id",
      "creative_set_id", "creative_instance_id",
      "advertiser_id",   "segment",
      "conversion_id",   "advertiser_public_key",
      "process_at",      "was_processed"};

  CopyTableColumns(transaction, "conversion_queue", "conversion_queue_temp",
                   from_columns, to_columns,
                   /*should_drop=*/true);

  // Rename temporary table
  RenameTable(transaction, "conversion_queue_temp", "conversion_queue");
}

void MigrateToV29(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "UPDATE conversion_queue SET process_at = (CAST(process_at AS INT64) + "
      "11644473600) * 1000000;";
  transaction->commands.push_back(std::move(command));
}

void MigrateToV30(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  // Create a temporary table with a new |type| column defaulted to
  // |kViewThroughConversionActionType| for legacy conversions, rename the
  // |conversion_id| column to |verifiable_conversion_id| and
  // rename the |advertiser_public_key| column to
  // |verifiable_advertiser_public_key|.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      "CREATE TABLE conversion_queue_temp (id INTEGER PRIMARY KEY "
      "AUTOINCREMENT NOT NULL, ad_type TEXT, campaign_id TEXT, creative_set_id "
      "TEXT NOT NULL, creative_instance_id TEXT NOT NULL, advertiser_id TEXT, "
      "segment TEXT, type TEXT NOT NULL DEFAULT '$1', verifiable_conversion_id "
      "TEXT, verifiable_advertiser_public_key TEXT, process_at TIMESTAMP NOT "
      "NULL, was_processed INTEGER DEFAULT 0);",
      {kViewThroughConversionActionType}, nullptr);
  transaction->commands.push_back(std::move(command));

  // Copy columns to temporary table
  const std::vector<std::string> from_columns = {
      "ad_type",         "campaign_id",
      "creative_set_id", "creative_instance_id",
      "advertiser_id",   "segment",
      "conversion_id",   "advertiser_public_key",
      "process_at",      "was_processed"};

  const std::vector<std::string> to_columns = {
      "ad_type",
      "campaign_id",
      "creative_set_id",
      "creative_instance_id",
      "advertiser_id",
      "segment",
      "verifiable_conversion_id",
      "verifiable_advertiser_public_key",
      "process_at",
      "was_processed"};

  CopyTableColumns(transaction, "conversion_queue", "conversion_queue_temp",
                   from_columns, to_columns, /*should_drop=*/true);

  // Rename temporary table
  RenameTable(transaction, "conversion_queue_temp", "conversion_queue");
}

}  // namespace

ConversionQueue::ConversionQueue() : batch_size_(kDefaultBatchSize) {}

void ConversionQueue::Save(
    const ConversionQueueItemList& conversion_queue_items,
    ResultCallback callback) const {
  if (conversion_queue_items.empty()) {
    return std::move(callback).Run(/*success=*/true);
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  const std::vector<ConversionQueueItemList> batches =
      SplitVector(conversion_queue_items, batch_size_);

  for (const auto& batch : batches) {
    InsertOrUpdate(&*transaction, batch);
  }

  RunTransaction(std::move(transaction), std::move(callback));
}

void ConversionQueue::Delete(
    const ConversionQueueItemInfo& conversion_queue_item,
    ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      "DELETE FROM $1 WHERE creative_instance_id = '$2';",
      {GetTableName(), conversion_queue_item.conversion.creative_instance_id},
      nullptr);
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

void ConversionQueue::Update(
    const ConversionQueueItemInfo& conversion_queue_item,
    ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      "UPDATE $1 SET was_processed = 1 WHERE was_processed == 0 AND "
      "creative_instance_id == '$2';",
      {GetTableName(), conversion_queue_item.conversion.creative_instance_id},
      nullptr);
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

void ConversionQueue::GetAll(GetConversionQueueCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
      "SELECT cq.ad_type, cq.campaign_id, cq.creative_set_id, "
      "cq.creative_instance_id, cq.advertiser_id, cq.segment, cq.type, "
      "cq.verifiable_conversion_id, cq.verifiable_advertiser_public_key, "
      "cq.process_at, cq.was_processed FROM $1 AS cq ORDER BY process_at ASC;",
      {GetTableName()}, nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void ConversionQueue::GetUnprocessed(
    GetConversionQueueCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
      "SELECT cq.ad_type, cq.campaign_id, cq.creative_set_id, "
      "cq.creative_instance_id, cq.advertiser_id, cq.segment, cq.type, "
      "cq.verifiable_conversion_id, cq.verifiable_advertiser_public_key, "
      "cq.process_at, cq.was_processed FROM $1 AS cq WHERE was_processed == 0 "
      "ORDER BY process_at ASC;",
      {GetTableName()}, nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetCallback, std::move(callback)));
}

void ConversionQueue::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetConversionQueueForCreativeInstanceIdCallback callback) const {
  if (creative_instance_id.empty()) {
    return std::move(callback).Run(/*success=*/false, creative_instance_id,
                                   /*conversion_queue_items=*/{});
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
      "SELECT cq.ad_type, cq.campaign_id, cq.creative_set_id, "
      "cq.creative_instance_id, cq.advertiser_id, cq.segment, cq.type, "
      "cq.verifiable_conversion_id, cq.verifiable_advertiser_public_key, "
      "cq.process_at, cq.was_processed FROM $1 AS cq WHERE "
      "cq.creative_instance_id = '$2' ORDER BY process_at ASC;",
      {GetTableName(), creative_instance_id}, nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetForCreativeInstanceIdCallback,
                                  creative_instance_id, std::move(callback)));
}

std::string ConversionQueue::GetTableName() const {
  return kTableName;
}

void ConversionQueue::Create(mojom::DBTransactionInfo* transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      "CREATE TABLE conversion_queue (id INTEGER PRIMARY KEY AUTOINCREMENT NOT "
      "NULL, ad_type TEXT, campaign_id TEXT, creative_set_id TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, advertiser_id TEXT, segment TEXT, "
      "type TEXT NOT NULL, verifiable_conversion_id TEXT, "
      "verifiable_advertiser_public_key TEXT, process_at TIMESTAMP NOT NULL, "
      "was_processed INTEGER DEFAULT 0);";
  transaction->commands.push_back(std::move(command));
}

void ConversionQueue::Migrate(mojom::DBTransactionInfo* transaction,
                              const int to_version) {
  CHECK(transaction);

  switch (to_version) {
    case 10: {
      MigrateToV10(transaction);
      break;
    }

    case 11: {
      MigrateToV11(transaction);
      break;
    }

    case 17: {
      MigrateToV17(transaction);
      break;
    }

    case 21: {
      MigrateToV21(transaction);
      break;
    }

    case 26: {
      MigrateToV26(transaction);
      break;
    }

    case 28: {
      MigrateToV28(transaction);
      break;
    }

    case 29: {
      MigrateToV29(transaction);
      break;
    }

    case 30: {
      MigrateToV30(transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void ConversionQueue::InsertOrUpdate(
    mojom::DBTransactionInfo* transaction,
    const ConversionQueueItemList& conversion_queue_items) const {
  CHECK(transaction);

  if (conversion_queue_items.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = BuildInsertOrUpdateSql(&*command, conversion_queue_items);
  transaction->commands.push_back(std::move(command));
}

std::string ConversionQueue::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const ConversionQueueItemList& conversion_queue_items) const {
  CHECK(command);

  const size_t binded_parameters_count =
      BindParameters(command, conversion_queue_items);

  return base::ReplaceStringPlaceholders(
      "INSERT OR REPLACE INTO $1 (ad_type, campaign_id, creative_set_id, "
      "creative_instance_id, advertiser_id, segment, type, "
      "verifiable_conversion_id, verifiable_advertiser_public_key, process_at, "
      "was_processed) VALUES $2;",
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count=*/11, binded_parameters_count)},
      nullptr);
}

}  // namespace brave_ads::database::table
