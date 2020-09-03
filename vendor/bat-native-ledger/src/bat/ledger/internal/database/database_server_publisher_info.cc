/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/database/database_server_publisher_info.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace {

const char kTableName[] = "server_publisher_info";

}  // namespace

namespace ledger {
namespace database {

DatabaseServerPublisherInfo::DatabaseServerPublisherInfo(
    LedgerImpl* ledger) :
    DatabaseTable(ledger),
    banner_(std::make_unique<DatabaseServerPublisherBanner>(ledger)) {
}

DatabaseServerPublisherInfo::~DatabaseServerPublisherInfo() = default;

void DatabaseServerPublisherInfo::InsertOrUpdate(
    const type::ServerPublisherInfo& server_info,
    ledger::ResultCallback callback) {
  if (server_info.publisher_key.empty()) {
    BLOG(0, "Publisher key is empty");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = type::DBTransaction::New();

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
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
  banner_->InsertOrUpdate(transaction.get(), server_info);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, _1, callback));
}

void DatabaseServerPublisherInfo::GetRecord(
    const std::string& publisher_key,
    client::GetServerPublisherInfoCallback callback) {
  if (publisher_key.empty()) {
    BLOG(1, "Publisher key is empty");
    callback(nullptr);
    return;
  }

  // Get banner first as is not complex struct where ServerPublisherInfo is
  auto banner_callback =
      std::bind(&DatabaseServerPublisherInfo::OnGetRecordBanner,
          this,
          _1,
          publisher_key,
          callback);

  banner_->GetRecord(publisher_key, banner_callback);
}

void DatabaseServerPublisherInfo::OnGetRecordBanner(
    type::PublisherBannerPtr banner,
    const std::string& publisher_key,
    client::GetServerPublisherInfoCallback callback) {
  auto transaction = type::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "SELECT status, address, updated_at "
      "FROM %s WHERE publisher_key=?",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  command->record_bindings = {
      type::DBCommand::RecordBindingType::INT_TYPE,
      type::DBCommand::RecordBindingType::STRING_TYPE,
      type::DBCommand::RecordBindingType::INT64_TYPE
  };

  transaction->commands.push_back(std::move(command));

  if (!banner) {
    banner = type::PublisherBanner::New();
  }

  auto transaction_callback =
      std::bind(&DatabaseServerPublisherInfo::OnGetRecord,
          this,
          _1,
          publisher_key,
          *banner,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabaseServerPublisherInfo::OnGetRecord(
    type::DBCommandResponsePtr response,
    const std::string& publisher_key,
    const type::PublisherBanner& banner,
    client::GetServerPublisherInfoCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(nullptr);
    return;
  }

  if (response->result->get_records().size() != 1) {
    callback(nullptr);
    return;
  }

  auto* record = response->result->get_records()[0].get();

  auto info = type::ServerPublisherInfo::New();
  info->publisher_key = publisher_key;
  info->status = static_cast<type::PublisherStatus>(
      GetIntColumn(record, 0));
  info->address = GetStringColumn(record, 1);
  info->updated_at = GetInt64Column(record, 2);
  info->banner = banner.Clone();

  callback(std::move(info));
}

void DatabaseServerPublisherInfo::DeleteExpiredRecords(
    const int64_t max_age_seconds,
    ledger::ResultCallback callback) {
  int64_t cutoff =
      util::GetCurrentTimeStamp() - max_age_seconds;

  auto transaction = type::DBTransaction::New();

  // Get a list of publisher keys that are older than |max_age_seconds|.
  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = base::StringPrintf(
      "SELECT publisher_key FROM %s WHERE updated_at < ?",
      kTableName);
  BindInt64(command.get(), 0, cutoff);

  command->record_bindings = {
      type::DBCommand::RecordBindingType::STRING_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto select_callback =
      std::bind(&DatabaseServerPublisherInfo::OnExpiredRecordsSelected,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      select_callback);
}

void DatabaseServerPublisherInfo::OnExpiredRecordsSelected(
    type::DBCommandResponsePtr response,
    ledger::ResultCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Unable to query for expired records");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  std::vector<std::string> publisher_keys;
  for (auto const& record : response->result->get_records()) {
    publisher_keys.push_back(GetStringColumn(record.get(), 0));
  }

  // Exit if there are no records to delete.
  if (publisher_keys.empty()) {
    callback(type::Result::LEDGER_OK);
    return;
  }

  std::string publisher_key_list = GenerateStringInCase(publisher_keys);

  auto transaction = type::DBTransaction::New();

  // Delete records in child tables.
  banner_->DeleteRecords(transaction.get(), publisher_key_list);

  // Delete records in this table.
  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "DELETE FROM %s WHERE publisher_key IN (%s)",
      kTableName,
      publisher_key_list.c_str());

  transaction->commands.push_back(std::move(command));

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, _1, callback));
}

}  // namespace database
}  // namespace ledger
