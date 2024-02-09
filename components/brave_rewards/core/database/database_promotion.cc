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
#include "brave/components/brave_rewards/core/database/database_promotion.h"
#include "brave/components/brave_rewards/core/database/database_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {
namespace database {

namespace {

const char kTableName[] = "promotion";

}  // namespace

DatabasePromotion::DatabasePromotion(RewardsEngineImpl& engine)
    : DatabaseTable(engine) {}

DatabasePromotion::~DatabasePromotion() = default;

void DatabasePromotion::InsertOrUpdate(mojom::PromotionPtr info,
                                       LegacyResultCallback callback) {
  if (!info) {
    engine_->Log(FROM_HERE) << "Info is null";
    callback(mojom::Result::FAILED);
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(promotion_id, version, type, public_keys, suggestions, "
      "approximate_value, status, created_at, claimable_until, expires_at, "
      "claimed_at, legacy) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, info->id);
  BindInt(command.get(), 1, info->version);
  BindInt(command.get(), 2, static_cast<int>(info->type));
  BindString(command.get(), 3, info->public_keys);
  BindInt64(command.get(), 4, info->suggestions);
  BindDouble(command.get(), 5, info->approximate_value);
  BindInt(command.get(), 6, static_cast<int>(info->status));
  BindInt64(command.get(), 7, info->created_at);
  BindInt64(command.get(), 8, info->claimable_until);
  BindInt64(command.get(), 9, info->expires_at);
  BindInt64(command.get(), 10, info->claimed_at);
  BindBool(command.get(), 11, info->legacy_claimed);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabasePromotion::GetRecord(const std::string& id,
                                  GetPromotionCallback callback) {
  if (id.empty()) {
    engine_->Log(FROM_HERE) << "Id is empty";
    return callback({});
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT promotion_id, version, type, public_keys, suggestions, "
      "approximate_value, status, created_at, claimable_until, expires_at, "
      "claimed_at, claim_id, legacy "
      "FROM %s WHERE promotion_id=?",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  BindString(command.get(), 0, id);

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::BOOL_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabasePromotion::OnGetRecord, base::Unretained(this),
                     std::move(callback)));
}

void DatabasePromotion::OnGetRecord(GetPromotionCallback callback,
                                    mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    callback({});
    return;
  }

  if (response->result->get_records().size() != 1) {
    engine_->Log(FROM_HERE) << "Record size is not correct: "
                            << response->result->get_records().size();
    callback({});
    return;
  }

  auto* record = response->result->get_records()[0].get();
  auto info = mojom::Promotion::New();
  info->id = GetStringColumn(record, 0);
  info->version = GetIntColumn(record, 1);
  info->type = static_cast<mojom::PromotionType>(GetIntColumn(record, 2));
  info->public_keys = GetStringColumn(record, 3);
  info->suggestions = GetInt64Column(record, 4);
  info->approximate_value = GetDoubleColumn(record, 5);
  info->status = static_cast<mojom::PromotionStatus>(GetIntColumn(record, 6));
  info->created_at = GetInt64Column(record, 7);
  info->claimable_until = GetInt64Column(record, 8);
  info->expires_at = GetInt64Column(record, 9);
  info->claimed_at = GetInt64Column(record, 10);
  info->claim_id = GetStringColumn(record, 11);
  info->legacy_claimed = GetBoolColumn(record, 12);

  callback(std::move(info));
}

void DatabasePromotion::GetAllRecords(GetAllPromotionsCallback callback) {
  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT "
      "promotion_id, version, type, public_keys, suggestions, "
      "approximate_value, status, created_at, claimable_until, expires_at, "
      "claimed_at, claim_id, legacy "
      "FROM %s",
      kTableName);

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::BOOL_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabasePromotion::OnGetAllRecords,
                     base::Unretained(this), std::move(callback)));
}

void DatabasePromotion::OnGetAllRecords(GetAllPromotionsCallback callback,
                                        mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    callback({});
    return;
  }

  base::flat_map<std::string, mojom::PromotionPtr> map;
  for (auto const& record : response->result->get_records()) {
    auto info = mojom::Promotion::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->version = GetIntColumn(record_pointer, 1);
    info->type =
        static_cast<mojom::PromotionType>(GetIntColumn(record_pointer, 2));
    info->public_keys = GetStringColumn(record_pointer, 3);
    info->suggestions = GetInt64Column(record_pointer, 4);
    info->approximate_value = GetDoubleColumn(record_pointer, 5);
    info->status =
        static_cast<mojom::PromotionStatus>(GetIntColumn(record_pointer, 6));
    info->created_at = GetInt64Column(record_pointer, 7);
    info->claimable_until = GetInt64Column(record_pointer, 8);
    info->expires_at = GetInt64Column(record_pointer, 9);
    info->claimed_at = GetInt64Column(record_pointer, 10);
    info->claim_id = GetStringColumn(record_pointer, 11);
    info->legacy_claimed = GetBoolColumn(record_pointer, 12);

    map.insert(std::make_pair(info->id, std::move(info)));
  }

  callback(std::move(map));
}

void DatabasePromotion::SaveClaimId(const std::string& promotion_id,
                                    const std::string& claim_id,
                                    LegacyResultCallback callback) {
  if (promotion_id.empty() || claim_id.empty()) {
    engine_->Log(FROM_HERE)
        << "Data is empty " << promotion_id << "/" << claim_id;
    callback(mojom::Result::FAILED);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s SET claim_id = ? WHERE promotion_id = ?", kTableName);

  auto transaction = mojom::DBTransaction::New();
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindString(command.get(), 0, claim_id);
  BindString(command.get(), 1, promotion_id);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabasePromotion::UpdateStatus(const std::string& promotion_id,
                                     mojom::PromotionStatus status,
                                     LegacyResultCallback callback) {
  if (promotion_id.empty()) {
    engine_->LogError(FROM_HERE) << "Promotion id is empty";
    callback(mojom::Result::FAILED);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s SET status = ? WHERE promotion_id = ?", kTableName);

  auto transaction = mojom::DBTransaction::New();
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(status));
  BindString(command.get(), 1, promotion_id);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabasePromotion::UpdateRecordsStatus(const std::vector<std::string>& ids,
                                            mojom::PromotionStatus status,
                                            LegacyResultCallback callback) {
  if (ids.empty()) {
    engine_->Log(FROM_HERE) << "List of ids is empty";
    callback(mojom::Result::FAILED);
    return;
  }

  const std::string query =
      base::StringPrintf("UPDATE %s SET status = ? WHERE promotion_id IN (%s)",
                         kTableName, GenerateStringInCase(ids).c_str());

  auto transaction = mojom::DBTransaction::New();
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  BindInt(command.get(), 0, static_cast<int>(status));

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabasePromotion::CredentialCompleted(const std::string& promotion_id,
                                            LegacyResultCallback callback) {
  if (promotion_id.empty()) {
    engine_->Log(FROM_HERE) << "Promotion id is empty";
    callback(mojom::Result::FAILED);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s SET status = ?, claimed_at = ? WHERE promotion_id = ?",
      kTableName);

  auto transaction = mojom::DBTransaction::New();
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = query;

  const uint64_t current_time = util::GetCurrentTimeStamp();

  BindInt(command.get(), 0, static_cast<int>(mojom::PromotionStatus::FINISHED));
  BindInt64(command.get(), 1, current_time);
  BindString(command.get(), 2, promotion_id);

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void DatabasePromotion::GetRecords(const std::vector<std::string>& ids,
                                   GetPromotionListCallback callback) {
  if (ids.empty()) {
    engine_->Log(FROM_HERE) << "List of ids is empty";
    callback({});
    return;
  }

  auto transaction = mojom::DBTransaction::New();

  const std::string query = base::StringPrintf(
      "SELECT "
      "promotion_id, version, type, public_keys, suggestions, "
      "approximate_value, status, created_at, claimable_until, expires_at, "
      "claimed_at, claim_id, legacy "
      "FROM %s WHERE promotion_id IN (%s)",
      kTableName, GenerateStringInCase(ids).c_str());

  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,
                              mojom::DBCommand::RecordBindingType::INT_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::INT64_TYPE,
                              mojom::DBCommand::RecordBindingType::STRING_TYPE,
                              mojom::DBCommand::RecordBindingType::BOOL_TYPE};

  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&DatabasePromotion::OnGetRecords, base::Unretained(this),
                     std::move(callback)));
}

void DatabasePromotion::OnGetRecords(GetPromotionListCallback callback,
                                     mojom::DBCommandResponsePtr response) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    engine_->LogError(FROM_HERE) << "Response is wrong";
    callback({});
    return;
  }

  std::vector<mojom::PromotionPtr> list;
  mojom::PromotionPtr info;
  for (auto const& record : response->result->get_records()) {
    info = mojom::Promotion::New();
    auto* record_pointer = record.get();

    info->id = GetStringColumn(record_pointer, 0);
    info->version = GetIntColumn(record_pointer, 1);
    info->type =
        static_cast<mojom::PromotionType>(GetIntColumn(record_pointer, 2));
    info->public_keys = GetStringColumn(record_pointer, 3);
    info->suggestions = GetInt64Column(record_pointer, 4);
    info->approximate_value = GetDoubleColumn(record_pointer, 5);
    info->status =
        static_cast<mojom::PromotionStatus>(GetIntColumn(record_pointer, 6));
    info->created_at = GetInt64Column(record_pointer, 7);
    info->claimable_until = GetInt64Column(record_pointer, 8);
    info->expires_at = GetInt64Column(record_pointer, 9);
    info->claimed_at = GetInt64Column(record_pointer, 10);
    info->claim_id = GetStringColumn(record_pointer, 11);
    info->legacy_claimed = GetBoolColumn(record_pointer, 12);

    list.push_back(std::move(info));
  }

  callback(std::move(list));
}

void DatabasePromotion::UpdateRecordsBlankPublicKey(
    const std::vector<std::string>& ids,
    LegacyResultCallback callback) {
  if (ids.empty()) {
    engine_->Log(FROM_HERE) << "List of ids is empty";
    callback(mojom::Result::FAILED);
    return;
  }

  const std::string query = base::StringPrintf(
      "UPDATE %s as p SET public_keys = "
      "(SELECT PRINTF('[\"%%s\"]', public_key) FROM creds_batch as cb "
      "WHERE cb.trigger_id = p.promotion_id) WHERE p.promotion_id IN (%s)",
      kTableName, GenerateStringInCase(ids).c_str());

  auto transaction = mojom::DBTransaction::New();
  auto command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;
  transaction->commands.push_back(std::move(command));

  engine_->client()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

}  // namespace database
}  // namespace brave_rewards::internal
