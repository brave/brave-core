/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/database/database_creds_batch.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal::database {

namespace {

constexpr char kTableName[] = "creds_batch";

}  // namespace

DatabaseCredsBatch::DatabaseCredsBatch(RewardsEngine& engine)
    : DatabaseTable(engine) {}

DatabaseCredsBatch::~DatabaseCredsBatch() = default;

void DatabaseCredsBatch::InsertOrUpdate(mojom::CredsBatchPtr creds,
                                        ResultCallback callback) {
  if (!creds) {
    engine_->Log(FROM_HERE) << "Creds is null";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(creds_id, trigger_id, trigger_type, creds, blinded_creds, "
      "signed_creds, public_key, batch_proof, status) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, creds->creds_id);
  BindString(command.get(), 1, creds->trigger_id);
  BindInt(command.get(), 2, static_cast<int>(creds->trigger_type));
  BindString(command.get(), 3, creds->creds);
  BindString(command.get(), 4, creds->blinded_creds);
  BindString(command.get(), 5, creds->signed_creds);
  BindString(command.get(), 6, creds->public_key);
  BindString(command.get(), 7, creds->batch_proof);
  BindInt(command.get(), 8, static_cast<int>(creds->status));

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseCredsBatch::GetRecordByTrigger(
    const std::string& trigger_id,
    const mojom::CredsBatchType trigger_type,
    GetCredsBatchCallback callback) {
  DCHECK(!trigger_id.empty());
  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT creds_id, trigger_id, trigger_type, creds, blinded_creds, "
      "signed_creds, public_key, batch_proof, status FROM %s "
      "WHERE trigger_id = ? AND trigger_type = ?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, trigger_id);
  BindInt(command.get(), 1, static_cast<int>(trigger_type));

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseCredsBatch::OnGetRecordByTrigger,
                     base::Unretained(this), std::move(callback)));
}

void DatabaseCredsBatch::OnGetRecordByTrigger(
    GetCredsBatchCallback callback,
    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
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

  auto info = mojom::CredsBatch::New();
  info->creds_id = GetStringColumn(record, 0);
  info->trigger_id = GetStringColumn(record, 1);
  info->trigger_type = CredsBatchTypeFromInt(GetIntColumn(record, 2));
  info->creds = GetStringColumn(record, 3);
  info->blinded_creds = GetStringColumn(record, 4);
  info->signed_creds = GetStringColumn(record, 5);
  info->public_key = GetStringColumn(record, 6);
  info->batch_proof = GetStringColumn(record, 7);
  info->status = CredsBatchStatusFromInt(GetIntColumn(record, 8));

  std::move(callback).Run(std::move(info));
}

void DatabaseCredsBatch::SaveSignedCreds(mojom::CredsBatchPtr creds,
                                         ResultCallback callback) {
  if (!creds) {
    engine_->Log(FROM_HERE) << "Creds is null";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET signed_creds = ?, public_key = ?, batch_proof = ?, "
      "status = ? WHERE trigger_id = ? AND trigger_type = ?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, creds->signed_creds);
  BindString(command.get(), 1, creds->public_key);
  BindString(command.get(), 2, creds->batch_proof);
  BindInt(command.get(), 3, static_cast<int>(mojom::CredsBatchStatus::SIGNED));
  BindString(command.get(), 4, creds->trigger_id);
  BindInt(command.get(), 5, static_cast<int>(creds->trigger_type));

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseCredsBatch::GetAllRecords(GetCredsBatchListCallback callback) {
  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT creds_id, trigger_id, trigger_type, creds, blinded_creds, "
      "signed_creds, public_key, batch_proof, status FROM %s",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseCredsBatch::OnGetRecords, base::Unretained(this),
                     std::move(callback)));
}

void DatabaseCredsBatch::OnGetRecords(GetCredsBatchListCallback callback,
                                      mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    std::move(callback).Run({});
    return;
  }

  std::vector<mojom::CredsBatchPtr> list;
  mojom::CredsBatchPtr info;
  for (auto const& record : response->result->get_records()) {
    auto* record_pointer = record.get();
    info = mojom::CredsBatch::New();

    info->creds_id = GetStringColumn(record_pointer, 0);
    info->trigger_id = GetStringColumn(record_pointer, 1);
    info->trigger_type = CredsBatchTypeFromInt(GetIntColumn(record_pointer, 2));
    info->creds = GetStringColumn(record_pointer, 3);
    info->blinded_creds = GetStringColumn(record_pointer, 4);
    info->signed_creds = GetStringColumn(record_pointer, 5);
    info->public_key = GetStringColumn(record_pointer, 6);
    info->batch_proof = GetStringColumn(record_pointer, 7);
    info->status = CredsBatchStatusFromInt(GetIntColumn(record_pointer, 8));
    list.push_back(std::move(info));
  }

  std::move(callback).Run(std::move(list));
}

void DatabaseCredsBatch::UpdateStatus(const std::string& trigger_id,
                                      mojom::CredsBatchType trigger_type,
                                      mojom::CredsBatchStatus status,
                                      ResultCallback callback) {
  if (trigger_id.empty()) {
    engine_->LogError(FROM_HERE) << "Trigger id is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET status = ? WHERE trigger_id = ? AND trigger_type = ?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(status));
  BindString(command.get(), 1, trigger_id);
  BindInt(command.get(), 2, static_cast<int>(trigger_type));

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseCredsBatch::UpdateRecordsStatus(
    const std::vector<std::string>& trigger_ids,
    mojom::CredsBatchType trigger_type,
    mojom::CredsBatchStatus status,
    ResultCallback callback) {
  if (trigger_ids.empty()) {
    engine_->LogError(FROM_HERE) << "Trigger id is empty";
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "UPDATE %s SET status = ? WHERE trigger_id IN (%s) AND trigger_type = ?",
      kTableName, GenerateStringInCase(trigger_ids).c_str());

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(status));
  BindInt(command.get(), 1, static_cast<int>(trigger_type));

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabaseCredsBatch::GetRecordsByTriggers(
    const std::vector<std::string>& trigger_ids,
    GetCredsBatchListCallback callback) {
  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT creds_id, trigger_id, trigger_type, creds, blinded_creds, "
      "signed_creds, public_key, batch_proof, status FROM %s "
      "WHERE trigger_id IN (%s)",
      kTableName, GenerateStringInCase(trigger_ids).c_str());

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabaseCredsBatch::OnGetRecords, base::Unretained(this),
                     std::move(callback)));
}

}  // namespace brave_rewards::internal::database
