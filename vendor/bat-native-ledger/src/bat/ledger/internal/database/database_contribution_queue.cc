/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/database/database_contribution_queue.h"
#include "bat/ledger/internal/database/database_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using std::placeholders::_1;

namespace ledger {
namespace database {

namespace {

const char kTableName[] = "contribution_queue";

}  // namespace

DatabaseContributionQueue::DatabaseContributionQueue(
    LedgerImpl* ledger) :
    DatabaseTable(ledger),
    publishers_(std::make_unique<DatabaseContributionQueuePublishers>(ledger)) {
}

DatabaseContributionQueue::~DatabaseContributionQueue() = default;

void DatabaseContributionQueue::InsertOrUpdate(
    mojom::ContributionQueuePtr info,
    ledger::LegacyResultCallback callback) {
  if (!info) {
    BLOG(0, "Queue is null");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  if (info->id.empty()) {
    BLOG(0, "Queue id is empty");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
    "INSERT OR REPLACE INTO %s (contribution_queue_id, type, amount, partial) "
    "VALUES (?, ?, ?, ?)",
    kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->id);
  BindInt(command.get(), 1, static_cast<int>(info->type));
  BindDouble(command.get(), 2, info->amount);
  BindBool(command.get(), 3, info->partial);

  transaction->commands.push_back(std::move(command));

  auto shared_info =
      std::make_shared<mojom::ContributionQueuePtr>(std::move(info));

  auto transaction_callback =
      std::bind(&DatabaseContributionQueue::OnInsertOrUpdate,
          this,
          _1,
          shared_info,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseContributionQueue::OnInsertOrUpdate(
    mojom::DBCommandResponsePtr response,
    std::shared_ptr<mojom::ContributionQueuePtr> shared_queue,
    ledger::LegacyResultCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is not ok");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  if (!shared_queue) {
    BLOG(0, "Queue is null");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  publishers_->InsertOrUpdate(
      (*shared_queue)->id,
      std::move((*shared_queue)->publishers),
      callback);
}

void DatabaseContributionQueue::GetFirstRecord(
    GetFirstContributionQueueCallback callback) {
  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT contribution_queue_id, type, amount, partial "
      "FROM %s WHERE completed_at = 0 "
      "ORDER BY created_at ASC LIMIT 1",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE};

  transaction->commands.push_back(std::move(command));

  auto transaction_callback =
      std::bind(&DatabaseContributionQueue::OnGetFirstRecord,
          this,
          _1,
          callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

void DatabaseContributionQueue::OnGetFirstRecord(
    mojom::DBCommandResponsePtr response,
    GetFirstContributionQueueCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Response is wrong");
    callback(nullptr);
    return;
  }

  if (response->result->get_records().size() != 1) {
    callback(nullptr);
    return;
  }

  auto* record = response->result->get_records()[0].get();

  auto info = mojom::ContributionQueue::New();
  info->id = GetStringColumn(record, 0);
  info->type = static_cast<mojom::RewardsType>(GetIntColumn(record, 1));
  info->amount = GetDoubleColumn(record, 2);
  info->partial = static_cast<bool>(GetIntColumn(record, 3));

  auto shared_info =
      std::make_shared<mojom::ContributionQueuePtr>(info->Clone());

  auto publishers_callback =
      std::bind(&DatabaseContributionQueue::OnGetPublishers,
          this,
          _1,
          shared_info,
          callback);

  publishers_->GetRecordsByQueueId(info->id, publishers_callback);
}

void DatabaseContributionQueue::OnGetPublishers(
    std::vector<mojom::ContributionQueuePublisherPtr> list,
    std::shared_ptr<mojom::ContributionQueuePtr> shared_queue,
    GetFirstContributionQueueCallback callback) {
  if (!shared_queue) {
    BLOG(0, "Queue is null");
    callback(nullptr);
    return;
  }

  (*shared_queue)->publishers = std::move(list);
  callback(std::move(*shared_queue));
}

void DatabaseContributionQueue::MarkRecordAsComplete(
    const std::string& id,
    ledger::LegacyResultCallback callback) {
  if (id.empty()) {
    BLOG(1, "Id is empty");
    callback(mojom::Result::LEDGER_ERROR);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET completed_at = ? WHERE contribution_queue_id = ?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindInt64(command.get(), 0, util::GetCurrentTimeStamp());
  BindString(command.get(), 1, id);

  transaction->commands.push_back(std::move(command));

  auto transaction_callback = std::bind(&OnResultCallback,
      _1,
      callback);

  ledger_->RunDBTransaction(std::move(transaction), transaction_callback);
}

}  // namespace database
}  // namespace ledger
