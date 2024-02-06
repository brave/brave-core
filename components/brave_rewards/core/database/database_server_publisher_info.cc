/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/database/database_server_publisher_info.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

using std::placeholders::_1;

namespace {

const char kTableName[] = "server_publisher_info";

}  // namespace

namespace brave_rewards::internal {

namespace database {

DatabaseServerPublisherInfo::DatabaseServerPublisherInfo(
    RewardsEngineImpl& engine)
    : DatabaseTable(engine), banner_(engine) {}

DatabaseServerPublisherInfo::~DatabaseServerPublisherInfo() = default;

void DatabaseServerPublisherInfo::InsertOrUpdate(
    const mojom::ServerPublisherInfo& server_info,
    LegacyResultCallback callback) {
  if (server_info.publisher_key.empty()) {
    engine_->LogError(FROM_HERE) << "Publisher key is empty";
    callback(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_key, status, address, updated_at) "
      "VALUES (?, ?, ?, ?)",
      kTableName);

  BindString(command.get(), 0, server_info.publisher_key);
  BindInt(command.get(), 1, static_cast<int>(server_info.status));
  BindString(command.get(), 2, server_info.address);
  BindInt64(command.get(), 3, server_info.updated_at);

  transaction->commands.push_back(std::move(command));
  banner_.InsertOrUpdate(transaction.get(), server_info);

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseServerPublisherInfo::GetRecord(
    const std::string& publisher_key,
    GetServerPublisherInfoCallback callback) {
  if (publisher_key.empty()) {
    engine_->Log(FROM_HERE) << "Publisher key is empty";
    callback(nullptr);
    return;
  }

  // Get banner first as is not complex struct where ServerPublisherInfo is
  auto banner_callback =
      std::bind(&DatabaseServerPublisherInfo::OnGetRecordBanner, this, _1,
                publisher_key, callback);

  banner_.GetRecord(publisher_key, banner_callback);
}

void DatabaseServerPublisherInfo::OnGetRecordBanner(
    mojom::PublisherBannerPtr banner,
    const std::string& publisher_key,
    GetServerPublisherInfoCallback callback) {
  auto transaction = mojom::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "SELECT status, address, updated_at "
      "FROM %s WHERE publisher_key=?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE};

  transaction->commands.push_back(std::move(command));

  if (!banner) {
    banner = mojom::PublisherBanner::New();
  }

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseServerPublisherInfo::OnGetRecord,
                     base::Unretained(this), std::move(callback), publisher_key,
                     std::move(banner)));
}

void DatabaseServerPublisherInfo::OnGetRecord(
    GetServerPublisherInfoCallback callback,
    const std::string& publisher_key,
    mojom::PublisherBannerPtr banner,
    mojom::DBCommandResponsePtr response) {
  CHECK(banner);

  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    callback(nullptr);
    return;
  }

  if (response->result->get_records().size() != 1) {
    callback(nullptr);
    return;
  }

  auto* record = response->result->get_records()[0].get();

  auto info = mojom::ServerPublisherInfo::New();
  info->publisher_key = publisher_key;
  info->status = static_cast<mojom::PublisherStatus>(GetIntColumn(record, 0));
  info->address = GetStringColumn(record, 1);
  info->updated_at = GetInt64Column(record, 2);
  info->banner = std::move(banner);

  callback(std::move(info));
}

void DatabaseServerPublisherInfo::DeleteExpiredRecords(
    int64_t max_age_seconds,
    LegacyResultCallback callback) {
  int64_t cutoff = util::GetCurrentTimeStamp() - max_age_seconds;

  auto transaction = mojom::DBTransaction::New();

  // Get a list of publisher keys that are older than |max_age_seconds|.
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = base::StringPrintf(
      "SELECT publisher_key FROM %s WHERE updated_at < ?", kTableName);
  BindInt64(command.get(), 0, cutoff);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseServerPublisherInfo::OnExpiredRecordsSelected,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseServerPublisherInfo::OnExpiredRecordsSelected(
    LegacyResultCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Unable to query for expired records";
    callback(mojom::Result::FAILED);
    return;
  }

  std::vector<std::string> publisher_keys;
  for (auto const& record : response->result->get_records()) {
    publisher_keys.push_back(GetStringColumn(record.get(), 0));
  }

  // Exit if there are no records to delete.
  if (publisher_keys.empty()) {
    callback(mojom::Result::OK);
    return;
  }

  std::string publisher_key_list = GenerateStringInCase(publisher_keys);

  auto transaction = mojom::DBTransaction::New();

  // Delete records in child tables.
  banner_.DeleteRecords(transaction.get(), publisher_key_list);

  // Delete records in this table.
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command =
      base::StringPrintf("DELETE FROM %s WHERE publisher_key IN (%s)",
                         kTableName, publisher_key_list.c_str());

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

}  // namespace database
}  // namespace brave_rewards::internal
