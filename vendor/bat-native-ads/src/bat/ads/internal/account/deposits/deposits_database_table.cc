/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/deposits/deposits_database_table.h"

#include <functional>
#include <utility>

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/database_statement_util.h"
#include "bat/ads/internal/base/database_table_util.h"
#include "bat/ads/internal/base/database_util.h"
#include "bat/ads/internal/base/logging_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace database {
namespace table {

namespace {

constexpr char kTableName[] = "deposits";

int BindParameters(mojom::DBCommand* command,
                   const CreativeAdList& creative_ads) {
  DCHECK(command);

  int count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindString(command, index++, creative_ad.creative_instance_id);
    BindDouble(command, index++, creative_ad.value);
    BindDouble(command, index++, creative_ad.end_at.ToDoubleT());

    count++;
  }

  return count;
}

void BindParameters(mojom::DBCommand* command, const DepositInfo& deposit) {
  DCHECK(command);
  DCHECK(deposit.IsValid());

  BindString(command, 0, deposit.creative_instance_id);
  BindDouble(command, 1, deposit.value);
  BindDouble(command, 2, deposit.expire_at.ToDoubleT());
}

DepositInfo GetFromRecord(mojom::DBRecord* record) {
  DCHECK(record);

  DepositInfo deposit;

  deposit.creative_instance_id = ColumnString(record, 0);
  deposit.value = ColumnDouble(record, 1);
  deposit.expire_at = base::Time::FromDoubleT(ColumnDouble(record, 2));

  return deposit;
}

}  // namespace

Deposits::Deposits() = default;

Deposits::~Deposits() = default;

void Deposits::Save(const DepositInfo& deposit, ResultCallback callback) {
  if (!deposit.IsValid()) {
    callback(/* success */ false);
    return;
  }

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();

  InsertOrUpdate(transaction.get(), deposit);

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

void Deposits::InsertOrUpdate(mojom::DBTransaction* transaction,
                              const CreativeAdList& creative_ads) {
  DCHECK(transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), creative_ads);

  transaction->commands.push_back(std::move(command));
}

void Deposits::InsertOrUpdate(mojom::DBTransaction* transaction,
                              const DepositInfo& deposit) {
  DCHECK(transaction);
  DCHECK(deposit.IsValid());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), deposit);

  transaction->commands.push_back(std::move(command));
}

void Deposits::GetForCreativeInstanceId(const std::string& creative_instance_id,
                                        GetDepositsCallback callback) {
  if (creative_instance_id.empty()) {
    callback(/* success */ false, absl::nullopt);
    return;
  }

  const std::string& query = base::StringPrintf(
      "SELECT "
      "creative_instance_id, "
      "value, "
      "expire_at "
      "FROM %s AS rv "
      "WHERE rv.creative_instance_id = '%s'",
      GetTableName().c_str(), creative_instance_id.c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommand::RecordBindingType::STRING_TYPE,  // creative_instance_id
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommand::RecordBindingType::DOUBLE_TYPE   // expire_at
  };

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&Deposits::OnGetForCreativeInstanceId, this,
                std::placeholders::_1, creative_instance_id, callback));
}

void Deposits::PurgeExpired(ResultCallback callback) {
  const std::string& query = base::StringPrintf(
      "DELETE FROM %s "
      "WHERE DATETIME('now') >= DATETIME(expire_at, 'unixepoch')",
      GetTableName().c_str());

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  mojom::DBTransactionPtr transaction = mojom::DBTransaction::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::Get()->RunDBTransaction(
      std::move(transaction),
      std::bind(&OnResultCallback, std::placeholders::_1, callback));
}

std::string Deposits::GetTableName() const {
  return kTableName;
}

void Deposits::Migrate(mojom::DBTransaction* transaction,
                       const int to_version) {
  DCHECK(transaction);

  switch (to_version) {
    case 24: {
      MigrateToV24(transaction);
      break;
    }

    default: {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

std::string Deposits::BuildInsertOrUpdateQuery(
    mojom::DBCommand* command,
    const CreativeAdList& creative_ads) {
  DCHECK(command);

  const int count = BindParameters(command, creative_ads);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(creative_instance_id, "
      "value, "
      "expire_at) VALUES %s",
      GetTableName().c_str(),
      BuildBindingParameterPlaceholders(3, count).c_str());
}

std::string Deposits::BuildInsertOrUpdateQuery(mojom::DBCommand* command,
                                               const DepositInfo& deposit) {
  DCHECK(command);
  DCHECK(deposit.IsValid());

  BindParameters(command, deposit);

  return base::StringPrintf(
      "INSERT OR REPLACE INTO %s "
      "(creative_instance_id, "
      "value, "
      "expire_at) VALUES %s",
      GetTableName().c_str(), BuildBindingParameterPlaceholders(3, 1).c_str());
}

void Deposits::OnGetForCreativeInstanceId(
    mojom::DBCommandResponsePtr response,
    const std::string& creative_instance_id,
    GetDepositsCallback callback) {
  if (!response ||
      response->status != mojom::DBCommandResponse::Status::RESPONSE_OK) {
    BLOG(0, "Failed to get deposit value");
    callback(/* success */ false, absl::nullopt);
    return;
  }

  if (response->result->get_records().empty()) {
    callback(/* success */ true, absl::nullopt);
    return;
  }

  mojom::DBRecordPtr record =
      std::move(response->result->get_records().front());
  const DepositInfo& deposit = GetFromRecord(record.get());

  callback(/* success */ true, deposit);
}

void Deposits::MigrateToV24(mojom::DBTransaction* transaction) {
  DCHECK(transaction);

  const std::string& query =
      "CREATE TABLE IF NOT EXISTS deposits "
      "(creative_instance_id TEXT NOT NULL, "
      "value DOUBLE NOT NULL, "
      "expire_at TIMESTAMP NOT NULL, "
      "PRIMARY KEY (creative_instance_id), "
      "UNIQUE(creative_instance_id) ON CONFLICT REPLACE)";

  mojom::DBCommandPtr command = mojom::DBCommand::New();
  command->type = mojom::DBCommand::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace table
}  // namespace database
}  // namespace ads
