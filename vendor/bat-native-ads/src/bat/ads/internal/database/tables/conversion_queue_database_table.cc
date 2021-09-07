/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/conversion_queue_database_table.h"

#include <functional>
#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
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

  const std::vector<ConversionQueueItemList> batches =
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

  const std::string query = base::StringPrintf(
      "DELETE FROM %s "
      "WHERE creative_instance_id = '%s'",
      get_table_name().c_str(),
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
  const std::string query = base::StringPrintf(
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
      get_table_name().c_str());

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
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE  // timestamp
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
  ConversionQueueItemList conversion_queue_items;

  if (creative_instance_id.empty()) {
    callback(/* success */ false, creative_instance_id, conversion_queue_items);
    return;
  }

  const std::string query = base::StringPrintf(
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
      get_table_name().c_str(), creative_instance_id.c_str());

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
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE  // timestamp
  };

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&ConversionQueue::OnGetForCreativeInstanceId, this,
                std::placeholders::_1, creative_instance_id, callback));
}

void ConversionQueue::set_batch_size(const int batch_size) {
  DCHECK_GT(batch_size, 0);

  batch_size_ = batch_size;
}

std::string ConversionQueue::get_table_name() const {
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

int ConversionQueue::BindParameters(
    mojom::DBCommand* command,
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
    BindDouble(command, index++, conversion_queue_item.timestamp.ToDoubleT());

    count++;
  }

  return count;
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
      get_table_name().c_str(),
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
    ConversionQueueItemInfo info = GetFromRecord(record.get());
    conversion_queue_items.push_back(info);
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
    ConversionQueueItemInfo info = GetFromRecord(record.get());
    conversion_queue_items.push_back(info);
  }

  callback(/* success */ true, creative_instance_id, conversion_queue_items);
}

ConversionQueueItemInfo ConversionQueue::GetFromRecord(
    mojom::DBRecord* record) const {
  ConversionQueueItemInfo info;

  info.campaign_id = ColumnString(record, 0);
  info.creative_set_id = ColumnString(record, 1);
  info.creative_instance_id = ColumnString(record, 2);
  info.advertiser_id = ColumnString(record, 3);
  info.conversion_id = ColumnString(record, 4);
  info.advertiser_public_key = ColumnString(record, 5);
  info.timestamp = base::Time::FromDoubleT(ColumnDouble(record, 6));

  return info;
}

void ConversionQueue::CreateTableV10(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  // campaign_id and advertiser_id can be NULL for legacy conversions migrated
  // from |ad_conversions.json| and conversion_id and advertiser_public_key will
  // be empty for non verifiable conversions
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s "
      "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "campaign_id TEXT, "
      "creative_set_id TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, "
      "advertiser_id TEXT, "
      "conversion_id TEXT, "
      "timestamp TIMESTAMP NOT NULL)",
      get_table_name().c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

void ConversionQueue::MigrateToV10(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  util::Drop(transaction, get_table_name());

  CreateTableV10(transaction);
}

void ConversionQueue::MigrateToV11(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string temp_table_name =
      base::StringPrintf("%s_temp", get_table_name().c_str());

  // Create a temporary table with new |advertiser_public_key| column
  const std::string query = base::StringPrintf(
      "CREATE TABLE %s "
      "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
      "campaign_id TEXT, "
      "creative_set_id TEXT NOT NULL, "
      "creative_instance_id TEXT NOT NULL, "
      "advertiser_id TEXT, "
      "conversion_id TEXT, "
      "advertiser_public_key TEXT, "
      "timestamp TIMESTAMP NOT NULL)",
      temp_table_name.c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  // Copy columns to temporary table
  const std::vector<std::string> columns = {
      "campaign_id",   "creative_set_id", "creative_instance_id",
      "advertiser_id", "conversion_id",   "timestamp"};

  util::CopyColumns(transaction, get_table_name(), temp_table_name, columns,
                    /* should_drop */ true);

  // Rename temporary table
  util::Rename(transaction, temp_table_name, get_table_name());
}

}  // namespace table
}  // namespace database
}  // namespace ads
