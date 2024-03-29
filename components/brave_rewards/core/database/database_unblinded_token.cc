/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <map>
#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/database/database_unblinded_token.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::database {

namespace {

constexpr char kTableName[] = "unblinded_tokens";

}  // namespace

DatabaseUnblindedToken::DatabaseUnblindedToken(RewardsEngine& engine)
    : DatabaseTable(engine) {}

DatabaseUnblindedToken::~DatabaseUnblindedToken() = default;

void DatabaseUnblindedToken::InsertOrUpdateList(
    std::vector<mojom::UnblindedTokenPtr> list,
    ResultCallback callback) {
  if (list.empty()) {
    engine_->Log(FROM_HERE) << "List is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR IGNORE INTO %s "
      "(token_id, token_value, public_key, value, creds_id, expires_at) "
      "VALUES (?, ?, ?, ?, ?, ?)",
      kTableName);

  for (const auto& info : list) {
    auto command = mojom::DBCommand::New();
    command->type = mojom::DBCommand::Type::kRun;
    command->command = query;

    if (info->id != 0) {
      BindInt64(command.get(), 0, info->id);
    } else {
      BindNull(command.get(), 0);
    }

    BindString(command.get(), 1, info->token_value);
    BindString(command.get(), 2, info->public_key);
    BindDouble(command.get(), 3, info->value);
    BindString(command.get(), 4, info->creds_id);
    BindInt64(command.get(), 5, info->expires_at);

    transaction->commands.push_back(std::move(command));
  }

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseUnblindedToken::OnGetRecords(
    GetUnblindedTokenListCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::kSuccess) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    std::move(callback).Run({});
    return;
  }

  std::vector<mojom::UnblindedTokenPtr> list;
  for (auto const& record : response->records) {
    auto info = mojom::UnblindedToken::New();
    auto* record_pointer = record.get();

    info->id = GetInt64Column(record_pointer, 0);
    info->token_value = GetStringColumn(record_pointer, 1);
    info->public_key = GetStringColumn(record_pointer, 2);
    info->value = GetDoubleColumn(record_pointer, 3);
    info->creds_id = GetStringColumn(record_pointer, 4);
    info->expires_at = GetInt64Column(record_pointer, 5);

    list.push_back(std::move(info));
  }

  std::move(callback).Run(std::move(list));
}

void DatabaseUnblindedToken::GetSpendableRecords(
    GetUnblindedTokenListCallback callback) {
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRead;
  command->command = base::StringPrintf(
      R"(
    SELECT
      ut.token_id,
      ut.token_value,
      ut.public_key,
      ut.value,
      ut.creds_id,
      ut.expires_at
    FROM
      %s as ut
    INNER JOIN
      creds_batch as cb ON cb.creds_id = ut.creds_id
    WHERE
      ut.redeemed_at = 0 AND cb.trigger_type = 1
      )",
      kTableName);
  command->record_bindings = {mojom::DBCommand::RecordBindingType::kInt64,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kDouble,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kInt64};

  auto transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseUnblindedToken::OnGetRecords,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseUnblindedToken::MarkRecordListAsSpent(
    const std::vector<std::string>& ids,
    mojom::RewardsType redeem_type,
    const std::string& redeem_id,
    ResultCallback callback) {
  if (ids.empty()) {
    engine_->Log(FROM_HERE) << "List of ids is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET redeemed_at = ?, redeem_id = ?, redeem_type = ? "
      "WHERE token_id IN (%s)",
      kTableName, GenerateStringInCase(ids).c_str());

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRun;
  command->command = query;

  BindInt64(command.get(), 0, util::GetCurrentTimeStamp());
  BindString(command.get(), 1, redeem_id);
  BindInt(command.get(), 2, static_cast<int>(redeem_type));

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseUnblindedToken::MarkRecordListAsReserved(
    const std::vector<std::string>& ids,
    const std::string& redeem_id,
    ResultCallback callback) {
  if (ids.empty()) {
    engine_->Log(FROM_HERE) << "List of ids is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string id_values = GenerateStringInCase(ids);

  std::string query = base::StringPrintf(
      "UPDATE %s SET redeem_id = ?, reserved_at = ? "
      "WHERE ( "
      "SELECT COUNT(*) FROM %s "
      "WHERE reserved_at = 0 AND redeemed_at = 0 AND token_id IN (%s) "
      ") = ? AND token_id IN (%s)",
      kTableName, kTableName, id_values.c_str(), id_values.c_str());

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRun;
  command->command = query;

  BindString(command.get(), 0, redeem_id);
  BindInt64(command.get(), 1, util::GetCurrentTimeStamp());
  BindInt64(command.get(), 2, ids.size());

  transaction->commands.push_back(std::move(command));

  query =
      "UPDATE contribution_info SET step=?, retry_count=0 "
      "WHERE contribution_id = ?";

  command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRun;
  command->command = query;

  BindInt(command.get(), 0,
          static_cast<int>(mojom::ContributionStep::STEP_RESERVE));
  BindString(command.get(), 1, redeem_id);

  transaction->commands.push_back(std::move(command));

  query = base::StringPrintf(
      "SELECT token_id FROM %s "
      "WHERE reserved_at != 0 AND token_id IN (%s)",
      kTableName, id_values.c_str());

  command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRead;
  command->command = query;

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseUnblindedToken::OnMarkRecordListAsReserved,
                     base::Unretained(this), std::move(callback), ids.size()));
}

void DatabaseUnblindedToken::OnMarkRecordListAsReserved(
    ResultCallback callback,
    size_t expected_row_count,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::kSuccess) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  if (response->records.size() != expected_row_count) {
    engine_->LogError(FROM_HERE) << "Records size doesn't match";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  std::move(callback).Run(mojom::Result::OK);
}

void DatabaseUnblindedToken::MarkRecordListAsSpendable(
    const std::string& redeem_id,
    ResultCallback callback) {
  if (redeem_id.empty()) {
    engine_->Log(FROM_HERE) << "Redeem id is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET redeem_id = '', reserved_at = 0 "
      "WHERE redeem_id = ? AND redeemed_at = 0",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRun;
  command->command = query;

  BindString(command.get(), 0, redeem_id);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseUnblindedToken::GetReservedRecordList(
    const std::string& redeem_id,
    GetUnblindedTokenListCallback callback) {
  if (redeem_id.empty()) {
    engine_->Log(FROM_HERE) << "Redeem id is empty";
    std::move(callback).Run({});
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT ut.token_id, ut.token_value, ut.public_key, ut.value, "
      "ut.creds_id, ut.expires_at FROM %s as ut "
      "WHERE ut.redeem_id = ? AND ut.redeemed_at = 0 AND ut.reserved_at != 0",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRead;
  command->command = query;

  BindString(command.get(), 0, redeem_id);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::kInt64,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kDouble,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kInt64};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseUnblindedToken::OnGetRecords,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseUnblindedToken::GetSpendableRecordListByBatchTypes(
    const std::vector<mojom::CredsBatchType>& batch_types,
    GetUnblindedTokenListCallback callback) {
  if (batch_types.empty()) {
    engine_->Log(FROM_HERE) << "Batch types is empty";
    std::move(callback).Run({});
    return;
  }

  std::vector<std::string> in_case;

  for (const auto& type : batch_types) {
    in_case.push_back(std::to_string(static_cast<int>(type)));
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT ut.token_id, ut.token_value, ut.public_key, ut.value, "
      "ut.creds_id, ut.expires_at FROM %s as ut "
      "LEFT JOIN creds_batch as cb ON cb.creds_id = ut.creds_id "
      "WHERE ut.redeemed_at = 0 AND "
      "(ut.expires_at > strftime('%%s','now') OR ut.expires_at = 0) AND "
      "(cb.trigger_type IN (%s) OR ut.creds_id IS NULL)",
      kTableName, base::JoinString(in_case, ",").c_str());

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::kRead;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::kInt64,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kDouble,
                              mojom::DBCommand::RecordBindingType::kString,
                              mojom::DBCommand::RecordBindingType::kInt64};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseUnblindedToken::OnGetRecords,
                     base::Unretained(this), std::move(callback)));
}

}  // namespace brave_rewards::internal::database
