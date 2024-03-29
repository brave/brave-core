/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/database/database_contribution_queue.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::database {

namespace {

constexpr char kTableName[] = "contribution_queue";

}  // namespace

DatabaseContributionQueue::DatabaseContributionQueue(RewardsEngine& engine)
    : DatabaseTable(engine), publishers_(engine) {}

DatabaseContributionQueue::~DatabaseContributionQueue() = default;

void DatabaseContributionQueue::InsertOrUpdate(mojom::ContributionQueuePtr info,
                                               ResultCallback callback) {
  if (!info) {
    engine_->LogError(FROM_HERE) << "Queue is null";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  if (info->id.empty()) {
    engine_->LogError(FROM_HERE) << "Queue id is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s (contribution_queue_id, type, amount, "
      "partial) "
      "VALUES (?, ?, ?, ?)",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRun;
  command->command = query;

  BindString(command.get(), 0, info->id);
  BindInt(command.get(), 1, static_cast<int>(info->type));
  BindDouble(command.get(), 2, info->amount);
  BindBool(command.get(), 3, info->partial);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseContributionQueue::OnInsertOrUpdate,
                     base::Unretained(this), std::move(callback),
                     std::move(info)));
}

void DatabaseContributionQueue::OnInsertOrUpdate(
    ResultCallback callback,
    mojom::ContributionQueuePtr queue,
    mojom::DBCommandResponsePtr response) {
  CHECK(queue);

  if (!response ||
      response->status != mojom::DBCommandResponse::Status::kSuccess) {
    engine_->LogError(FROM_HERE) << "Response is not ok";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  publishers_.InsertOrUpdate(queue->id, std::move(queue->publishers),
                             std::move(callback));
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
  command->type = mojom::DBCommand::Type::kRead;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kInt,
                              mojom::DBCommand::RecordBindingType::kDouble,
                              mojom::DBCommand::RecordBindingType::kInt};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseContributionQueue::OnGetFirstRecord,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseContributionQueue::OnGetFirstRecord(
    GetFirstContributionQueueCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::kSuccess) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    std::move(callback).Run(nullptr);
    return;
  }

  if (response->records.size() != 1) {
    std::move(callback).Run(nullptr);
    return;
  }

  auto* record = response->records[0].get();

  auto info = mojom::ContributionQueue::New();
  info->id = GetStringColumn(record, 0);
  info->type = RewardsTypeFromInt(GetIntColumn(record, 1));
  info->amount = GetDoubleColumn(record, 2);
  info->partial = static_cast<bool>(GetIntColumn(record, 3));

  publishers_.GetRecordsByQueueId(
      info->id, base::BindOnce(&DatabaseContributionQueue::OnGetPublishers,
                               weak_factory_.GetWeakPtr(), info->Clone(),
                               std::move(callback)));
}

void DatabaseContributionQueue::OnGetPublishers(
    mojom::ContributionQueuePtr queue,
    GetFirstContributionQueueCallback callback,
    std::vector<mojom::ContributionQueuePublisherPtr> list) {
  if (!queue) {
    engine_->LogError(FROM_HERE) << "Queue is null";
    std::move(callback).Run(nullptr);
    return;
  }

  queue->publishers = std::move(list);
  std::move(callback).Run(std::move(queue));
}

void DatabaseContributionQueue::MarkRecordAsComplete(const std::string& id,
                                                     ResultCallback callback) {
  if (id.empty()) {
    engine_->Log(FROM_HERE) << "Id is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET completed_at = ? WHERE contribution_queue_id = ?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRun;
  command->command = query;

  BindInt64(command.get(), 0, util::GetCurrentTimeStamp());
  BindString(command.get(), 1, id);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

}  // namespace brave_rewards::internal::database
