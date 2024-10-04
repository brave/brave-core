/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/database/database_contribution_info_publishers.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::database {

namespace {

constexpr char kTableName[] = "contribution_info_publishers";

}  // namespace

DatabaseContributionInfoPublishers::DatabaseContributionInfoPublishers(
    RewardsEngine& engine)
    : DatabaseTable(engine) {}

DatabaseContributionInfoPublishers::~DatabaseContributionInfoPublishers() =
    default;

void DatabaseContributionInfoPublishers::InsertOrUpdate(
    mojom::DBTransaction* transaction,
    mojom::ContributionInfoPtr info) {
  DCHECK(transaction);

  if (!info) {
    engine_->Log(FROM_HERE) << "Info is null";
    return;
  }

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(contribution_id, publisher_key, total_amount, contributed_amount) "
      "VALUES (?, ?, ?, ?)",
      kTableName);

  for (const auto& publisher : info->publishers) {
    auto command = mojom::DBCommand::New();
    command->type = mojom::DBCommand::Type::RUN;
    command->command = query;

    BindString(command.get(), 0, publisher->contribution_id);
    BindString(command.get(), 1, publisher->publisher_key);
    BindDouble(command.get(), 2, publisher->total_amount);
    BindDouble(command.get(), 3, publisher->contributed_amount);
    transaction->commands.push_back(std::move(command));
  }
}

void DatabaseContributionInfoPublishers::GetRecordByContributionList(
    const std::vector<std::string>& contribution_ids,
    ContributionPublisherListCallback callback) {
  if (contribution_ids.empty()) {
    engine_->Log(FROM_HERE) << "Contribution ids is empty";
    std::move(callback).Run({});
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT contribution_id, publisher_key, total_amount, contributed_amount "
      "FROM %s WHERE contribution_id IN (%s)",
      kTableName, GenerateStringInCase(contribution_ids).c_str());

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(
          &DatabaseContributionInfoPublishers::OnGetRecordByContributionList,
          base::Unretained(this), std::move(callback)));
}

void DatabaseContributionInfoPublishers::OnGetRecordByContributionList(
    ContributionPublisherListCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Response is not ok";
    std::move(callback).Run({});
    return;
  }

  std::vector<mojom::ContributionPublisherPtr> list;
  for (auto const& record : response->result->get_records()) {
    auto info = mojom::ContributionPublisher::New();
    auto* record_pointer = record.get();

    info->contribution_id = GetStringColumn(record_pointer, 0);
    info->publisher_key = GetStringColumn(record_pointer, 1);
    info->total_amount = GetDoubleColumn(record_pointer, 2);
    info->contributed_amount = GetDoubleColumn(record_pointer, 3);

    list.push_back(std::move(info));
  }

  std::move(callback).Run(std::move(list));
}

void DatabaseContributionInfoPublishers::GetContributionPublisherPairList(
    const std::vector<std::string>& contribution_ids,
    ContributionPublisherPairListCallback callback) {
  if (contribution_ids.empty()) {
    engine_->Log(FROM_HERE) << "Contribution ids is empty";
    std::move(callback).Run({});
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT cip.contribution_id, cip.publisher_key, cip.total_amount, "
      "pi.name, pi.url, pi.favIcon, spi.status, spi.updated_at, pi.provider "
      "FROM %s as cip "
      "INNER JOIN publisher_info AS pi ON cip.publisher_key = pi.publisher_id "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = cip.publisher_key "
      "WHERE cip.contribution_id IN (%s)",
      kTableName, GenerateStringInCase(contribution_ids).c_str());

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseContributionInfoPublishers::
                         OnGetContributionPublisherInfoMap,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseContributionInfoPublishers::OnGetContributionPublisherInfoMap(
    ContributionPublisherPairListCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Response is not ok";
    std::move(callback).Run({});
    return;
  }

  std::vector<ContributionPublisherInfoPair> pair_list;
  for (auto const& record : response->result->get_records()) {
    auto publisher = mojom::PublisherInfo::New();
    auto* record_pointer = record.get();

    publisher->id = GetStringColumn(record_pointer, 1);
    publisher->weight = GetDoubleColumn(record_pointer, 2);
    publisher->name = GetStringColumn(record_pointer, 3);
    publisher->url = GetStringColumn(record_pointer, 4);
    publisher->favicon_url = GetStringColumn(record_pointer, 5);
    publisher->status =
        PublisherStatusFromInt(GetInt64Column(record_pointer, 6));
    publisher->status_updated_at = GetInt64Column(record_pointer, 7);
    publisher->provider = GetStringColumn(record_pointer, 8);

    pair_list.emplace_back(GetStringColumn(record_pointer, 0),
                           std::move(publisher));
  }

  std::move(callback).Run(std::move(pair_list));
}

void DatabaseContributionInfoPublishers::UpdateContributedAmount(
    const std::string& contribution_id,
    const std::string& publisher_key,
    ResultCallback callback) {
  if (contribution_id.empty() || publisher_key.empty()) {
    engine_->Log(FROM_HERE)
        << "Data is empty " << contribution_id << "/" << publisher_key;
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET contributed_amount="
      "(SELECT total_amount WHERE contribution_id = ? AND publisher_key = ?) "
      "WHERE contribution_id = ? AND publisher_key = ?;",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, contribution_id);
  BindString(command.get(), 1, publisher_key);
  BindString(command.get(), 2, contribution_id);
  BindString(command.get(), 3, publisher_key);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

}  // namespace brave_rewards::internal::database
