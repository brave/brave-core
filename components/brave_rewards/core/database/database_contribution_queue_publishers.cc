/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/database/database_contribution_queue_publishers.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::database {

namespace {

constexpr char kTableName[] = "contribution_queue_publishers";

}  // namespace

DatabaseContributionQueuePublishers::DatabaseContributionQueuePublishers(
    RewardsEngine& engine)
    : DatabaseTable(engine) {}

DatabaseContributionQueuePublishers::~DatabaseContributionQueuePublishers() =
    default;

void DatabaseContributionQueuePublishers::InsertOrUpdate(
    const std::string& id,
    std::vector<mojom::ContributionQueuePublisherPtr> list,
    ResultCallback callback) {
  if (id.empty() || list.empty()) {
    engine_->Log(FROM_HERE) << "Empty data";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(contribution_queue_id, publisher_key, amount_percent) VALUES (?, ?, ?)",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRun;
  command->command = query;

  for (const auto& publisher : list) {
    BindString(command.get(), 0, id);
    BindString(command.get(), 1, publisher->publisher_key);
    BindDouble(command.get(), 2, publisher->amount_percent);

    transaction->commands.push_back(command->Clone());
  }

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseContributionQueuePublishers::GetRecordsByQueueId(
    const std::string& queue_id,
    ContributionQueuePublishersListCallback callback) {
  if (queue_id.empty()) {
    engine_->Log(FROM_HERE) << "Queue id is empty";
    std::move(callback).Run({});
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT publisher_key, amount_percent "
      "FROM %s WHERE contribution_queue_id = ?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRead;
  command->command = query;

  BindString(command.get(), 0, queue_id);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kDouble};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(
          &DatabaseContributionQueuePublishers::OnGetRecordsByQueueId,
          base::Unretained(this), std::move(callback)));
}

void DatabaseContributionQueuePublishers::OnGetRecordsByQueueId(
    ContributionQueuePublishersListCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::kSuccess) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    std::move(callback).Run({});
    return;
  }

  std::vector<mojom::ContributionQueuePublisherPtr> list;
  for (auto const& record : response->records) {
    auto info = mojom::ContributionQueuePublisher::New();
    auto* record_pointer = record.get();

    info->publisher_key = GetStringColumn(record_pointer, 0);
    info->amount_percent = GetDoubleColumn(record_pointer, 1);

    list.push_back(std::move(info));
  }

  std::move(callback).Run(std::move(list));
}

}  // namespace brave_rewards::internal::database
