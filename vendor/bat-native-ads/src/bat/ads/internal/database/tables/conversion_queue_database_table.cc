/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/conversion_queue_database_table.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/container_util.h"
#include "bat/ads/internal/database/database_statement_util.h"
#include "bat/ads/internal/database/database_table_util.h"
#include "bat/ads/internal/database/database_util.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace database {
namespace table {

namespace {

const char kTableName[] = "conversion_queue";

const int kDefaultBatchSize = 50;

int BindParameters(mojom::DBCommand* command,
                   const ConversionQueueItemList& conversion_queue_items) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& conversion_queue_item : conversion_queue_items) {
    BindString(command, index++, conversion_queue_item.campaign_id);
    BindString(command, index++, conversion_queue_item.creative_set_id);
    BindString(command, index++, conversion_queue_item.creative_instance_id);
    BindString(command, index++, conversion_queue_item.advertiser_id);
    BindString(command, index++, conversion_queue_item.conversion_id);
    BindString(command, index++, conversion_queue_item.advertiser_public_key);
    BindDouble(command, index++, conversion_queue_item.confirm_at.ToDoubleT());

    count++;
  }

  return count;
}

ConversionQueueItemInfo GetFromRecord(mojom::DBRecord* record) {
  DCHECK(record);

  ConversionQueueItemInfo conversion_queue_item;

  conversion_queue_item.campaign_id = ColumnString(record, 0);
  conversion_queue_item.creative_set_id = ColumnString(record, 1);
  conversion_queue_item.creative_instance_id = ColumnString(record, 2);
  conversion_queue_item.advertiser_id = ColumnString(record, 3);
  conversion_queue_item.conversion_id = ColumnString(record, 4);
  conversion_queue_item.advertiser_public_key = ColumnString(record, 5);
  conversion_queue_item.confirm_at =
      base::Time::FromDoubleT(ColumnDouble(record, 6));

  return conversion_queue_item;
}

}  // namespace

ConversionQueue::ConversionQueue() : batch_size_(kDefaultBatchSize) {}

ConversionQueue::~ConversionQueue() = default;

void ConversionQueue::Save(
    const ConversionQueueItemList& conversion_queue_items,
    ResultCallback callback) {
  if (conversion_queue_items.empty()) {
    callback(/* success */ true);
    return;
  }

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();

  const std::vector<ConversionQueueItemList>& batches =
      SplitVector(conversion_queue_items, batch_size_);

  for (const auto& batch : batches) {
    InsertOrUpdate(transaction.get(), batch);
  }

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void ConversionQueue::Delete(
    const ConversionQueueItemInfo& conversion_queue_item,
    ResultCallback callback) {
  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();

  const std::string& query = base::StringPrintf(
      "DELETE FROM %s "
      "WHERE creative_instance_id = '%s'",
      GetTableName().c_str(),
      conversion_queue_item.creative_instance_id.c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void ConversionQueue::GetAll(GetConversionQueueCallback callback) {
  const std::string& query = base::StringPrintf(
      "SELECT "
      "cq.campaign_id, "
      "cq.creative_set_id, "
      "cq.creative_instance_id, "
      "cq.advertiser_id, "
      "cq.conversion_id, "
      "cq.advertiser_public_key, "
      "cq.timestamp "
      "FROM %s AS cq "
      "ORDER BY timestamp ASC",
      GetTableName().c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_instance_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // conversion_id
      mojom::DBCommand::RecordBindingType::
          STRING_TYPE,                                  // advertiser_public_key
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE  // confirm_at
  };

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction), std::bind(&ConversionQueue::OnGetAll, this,
                                        std::placeholders::_1, callback));
}

void ConversionQueue::GetForCreativeInstanceId(
    const std::string& creative_instance_id,
    GetConversionQueueForCreativeInstanceIdCallback callback) {
  if (creative_instance_id.empty()) {
    callback(/* success */ false, creative_instance_id, {});
    return;
  }

  const std::string& query = base::StringPrintf(
      "SELECT "
      "cq.campaign_id, "
      "cq.creative_set_id, "
      "cq.creative_instance_id, "
      "cq.advertiser_id, "
      "cq.conversion_id, "
      "cq.advertiser_public_key, "
      "cq.timestamp "
      "FROM %s AS cq "
      "WHERE cq.creative_instance_id = '%s' "
      "ORDER BY timestamp ASC",
      GetTableName().c_str(), creative_instance_id.c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // campaign_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_set_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_instance_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // advertiser_id
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // conversion_id
      mojom::DBCommand::RecordBindingType::
          STRING_TYPE,                                  // advertiser_public_key
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE  // confirm_at
  };

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&ConversionQueue::OnGetForCreativeInstanceId, this,
                std::placeholders::_1, creative_instance_id, callback));
}

std::string ConversionQueue::GetTableName() const {
  return kTableName;
}

void ConversionQueue::Migrate(mojom::DBTransaction* transaction,
                              const int to_version) {
  DCHECK(transaction);

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

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

void ConversionQueue::InsertOrUpdate(
    mojom::DBTransaction* transaction,
    const ConversionQueueItemList& conversion_queue_items) {
  DCHECK(transaction);

  if (conversion_queue_items.empty()) {
    return;
  }

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command =
      BuildInsertOrUpdateQuery(command.get(), conversion_queue_items);

  transaction->commands.push_back(std::move(command));
}

std::string ConversionQueue::BuildInsertOrUpdateQuery(
    mojom::DBCommand* command,
    const ConversionQueueItemList& conversion_queue_items) {
  DCHECK(command);

  const int count = BindParameters(command, conversion_queue_items);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(campaign_id, "
      "creative_set_id, "
      "creative_instance_id, "
      "advertiser_id, "
      "conversion_id, "
      "advertiser_public_key, "
      "timestamp) VALUES %s",
      GetTableName().c_str(),
      BuildBindingParameterPlaceholders(7, count).c_str());
}

void ConversionQueue::OnGetAll(mojom::DBCommandResponsePtr response,
                               GetConversionQueueCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get conversion queue");
    callback(/* success */ false, {});
    return;
  }

  ConversionQueueItemList conversion_queue_items;

  for (const auto& record : response->result->get_records()) {
    const ConversionQueueItemInfo& conversion_queue_item =
        GetFromRecord(record.get());
    conversion_queue_items.push_back(conversion_queue_item);
  }

  callback(/* success */ true, conversion_queue_items);
}

void ConversionQueue::OnGetForCreativeInstanceId(
    mojom::DBCommandResponsePtr response,
    const std::string& creative_instance_id,
    GetConversionQueueForCreativeInstanceIdCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get conversion queue");
    callback(/* success */ false, creative_instance_id, {});
    return;
  }

  ConversionQueueItemList conversion_queue_items;

  for (const auto& record : response->result->get_records()) {
    const ConversionQueueItemInfo& conversion_queue_item =
        GetFromRecord(record.get());
    conversion_queue_items.push_back(conversion_queue_item);
  }

  callback(/* success */ true, creative_instance_id, conversion_queue_items);
}

void ConversionQueue::MigrateToV10(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, "conversion_queue");

  // campaign_id and advertiser_id can be NULL for legacy conversions migrated
  // from |ad_conversions.json| and conversion_id and advertiser_public_key will
  // be empty for non verifiable conversions
  const std::string& query =
      "CREATE TABLE conversion_queue "
      "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "campaign_id TEXT, "
      "creative_set_id TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, "
      "advertiser_id TEXT, "
      "conversion_id TEXT, "
      "timestamp TIMESTAMP NOT NULL)";

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void ConversionQueue::MigrateToV11(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string& temp_table_name = "conversion_queue_temp";

  // Create a temporary table with new |advertiser_public_key| column
  const std::string& query =
      "CREATE TABLE conversion_queue_temp "
      "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "campaign_id TEXT, "
      "creative_set_id TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, "
      "advertiser_id TEXT, "
      "conversion_id TEXT, "
      "advertiser_public_key TEXT, "
      "timestamp TIMESTAMP NOT NULL)";

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  // Copy columns to temporary table
  const std::vector<std::string>& columns = {
      "campaign_id",   "creative_set_id", "creative_instance_id",
      "advertiser_id", "conversion_id",   "timestamp"};

  util::CopyColumns(transaction, "conversion_queue", temp_table_name, columns,
                    /* should_drop */ true);

  // Rename temporary table
  util::Rename(transaction, temp_table_name, "conversion_queue");
}

void ConversionQueue::MigrateToV17(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  util::CreateIndex(transaction, "conversion_queue", "creative_instance_id");
}

}  // namespace table
}  // namespace database
}  // namespace ads
