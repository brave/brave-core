/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
#include <utility>

#include "base/strings/stringprintf.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/database/database_contribution_info.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::database {

namespace {

constexpr char kTableName[] = "contribution_info";
constexpr char kChildTableName[] = "contribution_info_publishers";

}  // namespace

DatabaseContributionInfo::DatabaseContributionInfo(RewardsEngine& engine)
    : DatabaseTable(engine), publishers_(engine) {}

DatabaseContributionInfo::~DatabaseContributionInfo() = default;

void DatabaseContributionInfo::InsertOrUpdate(mojom::ContributionInfoPtr info,
                                              ResultCallback callback) {
  if (!info) {
    engine_->Log(FROM_HERE) << "Info is null";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto created_at = info->created_at;
  if (info->created_at == 0) {
    created_at = util::GetCurrentTimeStamp();
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(contribution_id, amount, type, step, retry_count, created_at, "
      "processor) "
      "VALUES (?, ?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->contribution_id);
  BindDouble(command.get(), 1, info->amount);
  BindInt(command.get(), 2, static_cast<int>(info->type));
  BindInt(command.get(), 3, static_cast<int>(info->step));
  BindInt(command.get(), 4, info->retry_count);
  BindInt64(command.get(), 5, created_at);
  BindInt(command.get(), 6, static_cast<int>(info->processor));

  transaction->commands.push_back(std::move(command));

  publishers_.InsertOrUpdate(transaction.get(), info->Clone());

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseContributionInfo::GetRecord(const std::string& contribution_id,
                                         GetContributionInfoCallback callback) {
  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT ci.contribution_id, ci.amount, ci.type, ci.step, ci.retry_count, "
      "ci.processor, ci.created_at "
      "FROM %s as ci "
      "WHERE ci.contribution_id = ?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, contribution_id);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseContributionInfo::OnGetRecord,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void DatabaseContributionInfo::OnGetRecord(
    GetContributionInfoCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Response is not ok";
    std::move(callback).Run(nullptr);
    return;
  }

  if (response->result->get_records().size() != 1) {
    engine_->Log(FROM_HERE) << "Record size is not correct: "
                            << response->result->get_records().size();
    std::move(callback).Run(nullptr);
    return;
  }

  auto* record = response->result->get_records()[0].get();

  auto info = mojom::ContributionInfo::New();
  info->contribution_id = GetStringColumn(record, 0);
  info->amount = GetDoubleColumn(record, 1);
  info->type = RewardsTypeFromInt(GetInt64Column(record, 2));
  info->step = ContributionStepFromInt(GetIntColumn(record, 3));
  info->retry_count = GetIntColumn(record, 4);
  info->processor = ContributionProcessorFromInt(GetIntColumn(record, 5));
  info->created_at = GetInt64Column(record, 6);

  publishers_.GetRecordByContributionList(
      {info->contribution_id},
      base::BindOnce(&DatabaseContributionInfo::OnGetPublishers,
                     weak_factory_.GetWeakPtr(), info->Clone(),
                     std::move(callback)));
}

void DatabaseContributionInfo::OnGetPublishers(
    mojom::ContributionInfoPtr contribution,
    GetContributionInfoCallback callback,
    std::vector<mojom::ContributionPublisherPtr> list) {
  if (!contribution) {
    engine_->Log(FROM_HERE) << "Contribution is null";
    std::move(callback).Run(nullptr);
    return;
  }

  contribution->publishers = std::move(list);
  std::move(callback).Run(std::move(contribution));
}

void DatabaseContributionInfo::GetAllRecords(
    ContributionInfoListCallback callback) {
  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT ci.contribution_id, ci.amount, ci.type, ci.step, ci.retry_count,"
      "ci.processor, ci.created_at "
      "FROM %s as ci ",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseContributionInfo::OnGetList,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void DatabaseContributionInfo::GetOneTimeTips(const mojom::ActivityMonth month,
                                              const int year,
                                              GetOneTimeTipsCallback callback) {
  if (year == 0) {
    engine_->Log(FROM_HERE) << "Year is 0";
    std::move(callback).Run({});
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
      "ci.amount, ci.created_at, spi.status, spi.updated_at, pi.provider "
      "FROM %s as ci "
      "INNER JOIN %s AS cp "
      "ON cp.contribution_id = ci.contribution_id "
      "INNER JOIN publisher_info AS pi ON cp.publisher_key = pi.publisher_id "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id "
      "WHERE strftime('%%m',  datetime(ci.created_at, 'unixepoch')) = ? AND "
      "strftime('%%Y', datetime(ci.created_at, 'unixepoch')) = ? "
      "AND ci.type = ? AND ci.step = ?",
      kTableName, kChildTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  const std::string formatted_month =
      base::StringPrintf("%02d", base::to_underlying(month));

  BindString(command.get(), 0, formatted_month);
  BindString(command.get(), 1, std::to_string(year));
  BindInt(command.get(), 2, static_cast<int>(mojom::RewardsType::ONE_TIME_TIP));
  BindInt(command.get(), 3,
          static_cast<int>(mojom::ContributionStep::STEP_COMPLETED));

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseContributionInfo::OnGetOneTimeTips,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void DatabaseContributionInfo::OnGetOneTimeTips(
    GetOneTimeTipsCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Response is not ok";
    std::move(callback).Run({});
    return;
  }

  std::vector<mojom::PublisherInfoPtr> list;
  for (auto const& record : response->result->get_records()) {
    auto info = mojom::PublisherInfo::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->name = GetStringColumn(record_pointer, 1);
    info->url = GetStringColumn(record_pointer, 2);
    info->favicon_url = GetStringColumn(record_pointer, 3);
    info->weight = GetDoubleColumn(record_pointer, 4);
    info->reconcile_stamp = GetInt64Column(record_pointer, 5);
    info->status = PublisherStatusFromInt(GetInt64Column(record_pointer, 6));
    info->status_updated_at = GetInt64Column(record_pointer, 7);
    info->provider = GetStringColumn(record_pointer, 8);

    list.push_back(std::move(info));
  }

  std::move(callback).Run(std::move(list));
}

void DatabaseContributionInfo::GetNotCompletedRecords(
    ContributionInfoListCallback callback) {
  auto transaction = mojom::DBTransaction::New();

  // It is possible for externally-funded (SKU-based) ACs to be stalled after
  // hitting the max number of retries. Attempt to revive these ACs if an
  // external transaction has already been submitted for their SKU order.
  // TODO(zenparsing): Remove this query once we support unlimited retries with
  // backoff for ACs.
  auto revive_command = mojom::DBCommand::New();
  revive_command->type = mojom::DBCommand::Type::RUN;
  revive_command->command = R"sql(
      UPDATE contribution_info SET step = 1, retry_count = 0
      WHERE contribution_id IN (
        SELECT ci.contribution_id
        FROM contribution_info ci
        INNER JOIN contribution_info_publishers cip
          ON cip.contribution_id = ci.contribution_id
        INNER JOIN sku_order so
          ON so.contribution_id = ci.contribution_id
        WHERE ci.step = -7 AND ci.type = 2 AND so.status = 2
        GROUP BY ci.contribution_id
        HAVING SUM(cip.contributed_amount) = 0)
  )sql";

  transaction->commands.push_back(std::move(revive_command));

  const std::string query = base::StringPrintf(
      "SELECT ci.contribution_id, ci.amount, ci.type, ci.step, ci.retry_count, "
      "ci.processor, ci.created_at "
      "FROM %s as ci WHERE ci.step > 0",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseContributionInfo::OnGetList,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void DatabaseContributionInfo::OnGetList(ContributionInfoListCallback callback,
                                         mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Response is not ok";
    std::move(callback).Run({});
    return;
  }

  if (response->result->get_records().empty()) {
    std::move(callback).Run({});
    return;
  }

  std::vector<mojom::ContributionInfoPtr> list;
  std::vector<std::string> contribution_ids;
  for (const auto& record : response->result->get_records()) {
    auto info = mojom::ContributionInfo::New();
    auto* record_pointer = record.get();

    info->contribution_id = GetStringColumn(record_pointer, 0);
    info->amount = GetDoubleColumn(record_pointer, 1);
    info->type = RewardsTypeFromInt(GetInt64Column(record_pointer, 2));
    info->step = ContributionStepFromInt(GetIntColumn(record_pointer, 3));
    info->retry_count = GetIntColumn(record_pointer, 4);
    info->processor =
        ContributionProcessorFromInt(GetIntColumn(record_pointer, 5));
    info->created_at = GetInt64Column(record_pointer, 6);

    contribution_ids.push_back(info->contribution_id);
    list.push_back(std::move(info));
  }

  publishers_.GetRecordByContributionList(
      contribution_ids,
      base::BindOnce(&DatabaseContributionInfo::OnGetListPublishers,
                     weak_factory_.GetWeakPtr(), std::move(list),
                     std::move(callback)));
}

void DatabaseContributionInfo::OnGetListPublishers(
    std::vector<mojom::ContributionInfoPtr> contributions,
    ContributionInfoListCallback callback,
    std::vector<mojom::ContributionPublisherPtr> list) {
  for (const auto& contribution : contributions) {
    for (const auto& item : list) {
      if (item->contribution_id != contribution->contribution_id) {
        continue;
      }

      contribution->publishers.push_back(item->Clone());
    }
  }

  std::move(callback).Run(std::move(contributions));
}

void DatabaseContributionInfo::UpdateStep(const std::string& contribution_id,
                                          mojom::ContributionStep step,
                                          ResultCallback callback) {
  if (contribution_id.empty()) {
    engine_->Log(FROM_HERE) << "Contribution id is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET step=?, retry_count=0 WHERE contribution_id = ?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(step));
  BindString(command.get(), 1, contribution_id);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseContributionInfo::UpdateStepAndCount(
    const std::string& contribution_id,
    mojom::ContributionStep step,
    int32_t retry_count,
    ResultCallback callback) {
  if (contribution_id.empty()) {
    engine_->Log(FROM_HERE) << "Contribution id is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET step=?, retry_count=? WHERE contribution_id = ?;",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(step));
  BindInt(command.get(), 1, retry_count);
  BindString(command.get(), 2, contribution_id);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseContributionInfo::UpdateContributedAmount(
    const std::string& contribution_id,
    const std::string& publisher_key,
    ResultCallback callback) {
  publishers_.UpdateContributedAmount(contribution_id, publisher_key,
                                      std::move(callback));
}

void DatabaseContributionInfo::FinishAllInProgressRecords(
    ResultCallback callback) {
  auto transaction = mojom::DBTransaction::New();
  const std::string query = base::StringPrintf(
      "UPDATE %s SET step = ?, retry_count = 0 WHERE step >= 0", kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0,
          static_cast<int>(mojom::ContributionStep::STEP_REWARDS_OFF));

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

}  // namespace brave_rewards::internal::database
