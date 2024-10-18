/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/database/database_recurring_tip.h"

#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/contribution/contribution.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::database {

namespace {

// TODO(https://github.com/brave/brave-browser/issues/7144):
//  rename to recurring_tip
constexpr char kTableName[] = "recurring_donation";

void MapDatabaseResultToSuccess(base::OnceCallback<void(bool)> callback,
                                mojom::DBCommandResponsePtr response) {
  std::move(callback).Run(response &&
                          response->status ==
                              mojom::DBCommandResponse::Status::RESPONSE_OK);
}

}  // namespace

DatabaseRecurringTip::DatabaseRecurringTip(RewardsEngine& engine)
    : DatabaseTable(engine) {}

DatabaseRecurringTip::~DatabaseRecurringTip() = default;

void DatabaseRecurringTip::InsertOrUpdate(mojom::RecurringTipPtr info,
                                          ResultCallback callback) {
  if (!info || info->publisher_key.empty()) {
    engine_->Log(FROM_HERE) << "Publisher key is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_id, amount, added_date) "
      "VALUES (?, ?, ?)",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->publisher_key);
  BindDouble(command.get(), 1, info->amount);
  BindInt64(command.get(), 2, info->created_at);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseRecurringTip::InsertOrUpdate(
    const std::string& publisher_id,
    double amount,
    base::OnceCallback<void(bool)> callback) {
  if (publisher_id.empty()) {
    engine_->Log(FROM_HERE) << "Publisher ID is empty";
    std::move(callback).Run(false);
    return;
  }

  if (amount <= 0) {
    engine_->Log(FROM_HERE) << "Invalid contribution amount";
    std::move(callback).Run(false);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(publisher_id, amount, added_date, next_contribution_at) "
      "VALUES (?, ?, ?, ?)",
      kTableName);

  base::Time added_at = base::Time::Now();
  base::Time next_date = added_at + base::Seconds(constant::kReconcileInterval);

  BindString(command.get(), 0, publisher_id);
  BindDouble(command.get(), 1, amount);
  BindInt64(command.get(), 2,
            static_cast<int64_t>(added_at.InSecondsFSinceUnixEpoch()));
  BindInt64(command.get(), 3,
            static_cast<int64_t>(next_date.InSecondsFSinceUnixEpoch()));

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(MapDatabaseResultToSuccess, std::move(callback)));
}

void DatabaseRecurringTip::AdvanceMonthlyContributionDates(
    const std::vector<std::string>& publisher_ids,
    base::OnceCallback<void(bool)> callback) {
  const std::string query = base::StringPrintf(
      "UPDATE %s SET next_contribution_at = ? WHERE publisher_id = ?",
      kTableName);

  auto next = base::Time::Now() + base::Seconds(constant::kReconcileInterval);

  auto transaction = mojom::DBTransaction::New();
  for (const std::string& publisher_id : publisher_ids) {
    if (!publisher_id.empty()) {
      auto command = mojom::DBCommand::New();
      command->type = mojom::DBCommand::Type::RUN;
      command->command = query;
      BindInt64(command.get(), 0,
                static_cast<int64_t>(next.InSecondsFSinceUnixEpoch()));
      BindString(command.get(), 1, publisher_id);
      transaction->commands.push_back(std::move(command));
    }
  }

  if (transaction->commands.empty()) {
    std::move(callback).Run(true);
    return;
  }

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(MapDatabaseResultToSuccess, std::move(callback)));
}

void DatabaseRecurringTip::GetNextMonthlyContributionTime(
    base::OnceCallback<void(std::optional<base::Time>)> callback) {
  auto transaction = mojom::DBTransaction::New();

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = base::StringPrintf(
      "UPDATE %s SET next_contribution_at = ? "
      "WHERE next_contribution_at IS NULL",
      kTableName);
  BindInt64(command.get(), 0, engine_->contribution()->GetReconcileStamp());
  transaction->commands.push_back(std::move(command));

  command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = base::StringPrintf(
      "SELECT MIN(next_contribution_at) FROM %s", kTableName);
  command->record_bindings = {mojom::DBCommand::RecordBindingType::INT64_TYPE};
  transaction->commands.push_back(std::move(command));

  auto on_completed =
      [](base::OnceCallback<void(std::optional<base::Time>)> callback,
         mojom::DBCommandResponsePtr response) {
        base::Time time;
        if (response &&
            response->status == mojom::DBCommandResponse::Status::RESPONSE_OK &&
            response->result && !response->result->get_records().empty()) {
          const auto& record = response->result->get_records().front();
          int64_t timestamp = GetInt64Column(record.get(), 0);
          if (timestamp > 0) {
            time = base::Time::FromSecondsSinceUnixEpoch(
                static_cast<double>(timestamp));
            if (time < base::Time::Now()) {
              time = base::Time::Now();
            }
            std::move(callback).Run(time);
            return;
          }
        }
        std::move(callback).Run(std::nullopt);
      };

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(on_completed, std::move(callback)));
}

void DatabaseRecurringTip::GetAllRecords(GetRecurringTipsCallback callback) {
  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT pi.publisher_id, pi.name, pi.url, pi.favIcon, "
      "rd.amount, rd.next_contribution_at, spi.status, spi.updated_at, "
      "pi.provider "
      "FROM %s as rd "
      "INNER JOIN publisher_info AS pi ON rd.publisher_id = pi.publisher_id "
      "LEFT JOIN server_publisher_info AS spi "
      "ON spi.publisher_key = pi.publisher_id ",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

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
      base::BindOnce(&DatabaseRecurringTip::OnGetAllRecords,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseRecurringTip::OnGetAllRecords(
    GetRecurringTipsCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
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

    // If a monthly contribution record does not have a valid "next contribution
    // date", then use the next auto-contribution date instead.
    if (!info->reconcile_stamp) {
      info->reconcile_stamp = engine_->contribution()->GetReconcileStamp();
    }

    list.push_back(std::move(info));
  }

  std::move(callback).Run(std::move(list));
}

void DatabaseRecurringTip::DeleteRecord(const std::string& publisher_key,
                                        ResultCallback callback) {
  if (publisher_key.empty()) {
    engine_->Log(FROM_HERE) << "Publisher key is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query =
      base::StringPrintf("DELETE FROM %s WHERE publisher_id = ?", kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, publisher_key);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

}  // namespace brave_rewards::internal::database
