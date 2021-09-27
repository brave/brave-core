/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/database/database_pending_contribution.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/constants.h"

using std::placeholders::_1;

namespace ledger {
namespace database {

namespace {

const char kTableName[] = "pending_contribution";

}  // namespace

DatabasePendingContribution::DatabasePendingContribution(
    LedgerImpl* ledger) :
    DatabaseTable(ledger) {
}

DatabasePendingContribution::~DatabasePendingContribution() = default;

void DatabasePendingContribution::InsertOrUpdateList(
    type::PendingContributionList list,
    ledger::ResultCallback callback) {
  if (list.empty()) {
    BLOG(1, "List is empty");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = type::DBTransaction::New();
  const uint64_t now = util::GetCurrentTimeStamp();

  const std::string query = base::StringPrintf(
      "INSERT INTO %s (pending_contribution_id, publisher_id, amount, "
      "added_date, viewing_id, type, processor) "
      "VALUES (?, ?, ?, ?, ?, ?, ?)",
      kTableName);

  for (const auto& item : list) {
    auto command = type::DBCommand::New();
    command->type = type::DBCommand::Type::RUN;
    command->command = query;

    BindNull(command.get(), 0);
    BindString(command.get(), 1, item->publisher_key);
    BindDouble(command.get(), 2, item->amount);
    BindInt64(command.get(), 3, now);
    BindString(command.get(), 4, item->viewing_id);
    BindInt(command.get(), 5, static_cast<int>(item->type));
    BindInt(command.get(), 6, static_cast<int>(item->processor));

    transaction->commands.push_back(std::move(command));
  }

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabasePendingContribution::GetReservedAmount(
    ledger::PendingContributionsTotalCallback callback) {
  auto transaction = type::DBTransaction::New();
  const std::string query = base::StringPrintf(
    "SELECT SUM(amount) FROM %s",
    kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      type::DBCommand::RecordBindingType::DOUBLE_TYPE
  };

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePendingContribution::OnGetReservedAmount,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabasePendingContribution::OnGetReservedAmount(
    type::DBCommandResponsePtr response,
    ledger::PendingContributionsTotalCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(0.0);
    return;
  }

  if (response->result->get_records().size() != 1) {
    callback(0.0);
    return;
  }

  auto* record = response->result->get_records()[0].get();

  callback(GetDoubleColumn(record, 0));
}

void DatabasePendingContribution::GetAllRecords(
    ledger::PendingContributionInfoListCallback callback) {
  auto transaction = type::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "SELECT pc.pending_contribution_id, pi.publisher_id, pi.name, "
      "pi.url, pi.favIcon, spi.status, spi.updated_at, pi.provider, "
      "pc.amount, pc.added_date, pc.viewing_id, pc.type, pc.processor "
      "FROM %s as pc "
      "INNER JOIN publisher_info AS pi ON pc.publisher_id = pi.publisher_id "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              type::DBCommand::RecordBindingType::INT64_TYPE,
                              type::DBCommand::RecordBindingType::STRING_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE,
                              type::DBCommand::RecordBindingType::INT_TYPE};

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabasePendingContribution::OnGetAllRecords,
          this,
          _1,
          callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabasePendingContribution::OnGetAllRecords(
    type::DBCommandResponsePtr response,
    ledger::PendingContributionInfoListCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback({});
    return;
  }

  type::PendingContributionInfoList list;
  for (auto const& record : response->result->get_records()) {
    auto info = type::PendingContributionInfo::New();
    auto* record_pointer = record.get();

    info->id = GetInt64Column(record_pointer, 0);
    info->publisher_key = GetStringColumn(record_pointer, 1);
    info->name = GetStringColumn(record_pointer, 2);
    info->url = GetStringColumn(record_pointer, 3);
    info->favicon_url = GetStringColumn(record_pointer, 4);
    info->status = static_cast<type::PublisherStatus>(
        GetInt64Column(record_pointer, 5));
    info->status_updated_at = GetInt64Column(record_pointer, 6);
    info->provider = GetStringColumn(record_pointer, 7);
    info->amount = GetDoubleColumn(record_pointer, 8);
    info->added_date = GetInt64Column(record_pointer, 9);
    info->viewing_id = GetStringColumn(record_pointer, 10);
    info->type = static_cast<type::RewardsType>(
        GetIntColumn(record_pointer, 11));
    info->expiration_date =
        info->added_date +
        constant::kPendingContributionExpirationInterval;
    info->processor = static_cast<type::ContributionProcessor>(
        GetIntColumn(record_pointer, 12));

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabasePendingContribution::GetUnverifiedPublishers(
    ledger::UnverifiedPublishersCallback callback) {
  std::string query = base::StringPrintf(
      "SELECT pi.publisher_id "
      "FROM %s AS pc "
      "INNER JOIN publisher_info AS pi ON pc.publisher_id = pi.publisher_id "
      "LEFT JOIN server_publisher_info AS spi ON spi.publisher_key = "
      "pi.publisher_id "
      "WHERE spi.status IS NULL OR spi.status IN (0, 1) "
      "GROUP BY pi.publisher_id",
      kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::READ;
  command->command = std::move(query);
  command->record_bindings = {type::DBCommand::RecordBindingType::STRING_TYPE};

  auto transaction = type::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      std::bind(&DatabasePendingContribution::OnGetUnverifiedPublishers, this,
                _1, std::move(callback)));
}

void DatabasePendingContribution::OnGetUnverifiedPublishers(
    type::DBCommandResponsePtr response,
    ledger::UnverifiedPublishersCallback callback) {
  if (!response ||
      response->status != type::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    return callback({});
  }

  std::vector<std::string> publisher_keys{};
  for (const auto& record : response->result->get_records()) {
    auto* record_pointer = record.get();
    DCHECK(record_pointer);
    publisher_keys.push_back(GetStringColumn(record_pointer, 0));
  }

  callback(std::move(publisher_keys));
}

void DatabasePendingContribution::DeleteRecord(
    const uint64_t id,
    ledger::ResultCallback callback) {
  if (id == 0) {
    BLOG(1, "Id is 0");
    callback(type::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = type::DBTransaction::New();
  const std::string query = base::StringPrintf(
    "DELETE FROM %s WHERE pending_contribution_id = ?",
    kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = query;

  BindInt64(command.get(), 0, id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

void DatabasePendingContribution::DeleteAllRecords(
    ledger::ResultCallback callback) {
  auto transaction = type::DBTransaction::New();
  const std::string query = base::StringPrintf("DELETE FROM %s", kTableName);

  auto command = type::DBCommand::New();
  command->type = type::DBCommand::Type::RUN;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->ledger_client()->RunDBTransaction(
      std::move(transaction),
      transaction_callback);
}

}  // namespace database
}  // namespace ledger
