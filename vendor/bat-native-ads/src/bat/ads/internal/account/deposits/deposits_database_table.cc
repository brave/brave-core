/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/deposits/deposits_database_table.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/strings/stringprintf.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/common/database/database_bind_util.h"
#include "bat/ads/internal/common/database/database_column_util.h"
#include "bat/ads/internal/common/database/database_transaction_util.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads::database::table {

namespace {

constexpr char kTableName[] = "deposits";

int BindParameters(mojom::DBCommandInfo* command,
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

void BindParameters(mojom::DBCommandInfo* command, const DepositInfo& deposit) {
  DCHECK(command);
  DCHECK(deposit.IsValid());

  BindString(command, 0, deposit.creative_instance_id);
  BindDouble(command, 1, deposit.value);
  BindDouble(command, 2, deposit.expire_at.ToDoubleT());
}

DepositInfo GetFromRecord(mojom::DBRecordInfo* record) {
  DCHECK(record);

  DepositInfo deposit;

  deposit.creative_instance_id = ColumnString(record, 0);
  deposit.value = ColumnDouble(record, 1);
  deposit.expire_at = base::Time::FromDoubleT(ColumnDouble(record, 2));

  return deposit;
}

void OnGetForCreativeInstanceId(const std::string& /*creative_instance_id*/,
                                GetDepositsCallback callback,
                                mojom::DBCommandResponseInfoPtr response) {
  if (!response || response->status !=
                       mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get deposit value");
    std::move(callback).Run(/*success*/ false, absl::nullopt);
    return;
  }

  if (response->result->get_records().empty()) {
    std::move(callback).Run(/*success*/ true, absl::nullopt);
    return;
  }

  const mojom::DBRecordInfoPtr record =
      std::move(response->result->get_records().front());
  DepositInfo deposit = GetFromRecord(record.get());

  std::move(callback).Run(/*success*/ true, std::move(deposit));
}

void MigrateToV24(mojom::DBTransactionInfo* transaction) {
  DCHECK(transaction);

  const std::string query =
      "CREATE TABLE IF NOT EXISTS deposits "
      "(creative_instance_id TEXT NOT NULL, "
      "value DOUBLE NOT NULL, "
      "expire_at TIMESTAMP NOT NULL, "
      "PRIMARY KEY (creative_instance_id), "
      "UNIQUE(creative_instance_id) ON CONFLICT REPLACE)";

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  transaction->commands.push_back(std::move(command));
}

}  // namespace

void Deposits::Save(const DepositInfo& deposit, ResultCallback callback) {
  if (!deposit.IsValid()) {
    std::move(callback).Run(/*success*/ false);
    return;
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  InsertOrUpdate(transaction.get(), deposit);

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

void Deposits::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                              const CreativeAdList& creative_ads) {
  DCHECK(transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), creative_ads);

  transaction->commands.push_back(std::move(command));
}

void Deposits::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                              const DepositInfo& deposit) {
  DCHECK(transaction);
  DCHECK(deposit.IsValid());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->command = BuildInsertOrUpdateQuery(command.get(), deposit);

  transaction->commands.push_back(std::move(command));
}

void Deposits::GetForCreativeInstanceId(const std::string& creative_instance_id,
                                        GetDepositsCallback callback) const {
  if (creative_instance_id.empty()) {
    std::move(callback).Run(/*success*/ false, absl::nullopt);
    return;
  }

  const std::string query = base::StringPrintf(
      "SELECT "
      "creative_instance_id, "
      "value, "
      "expire_at "
      "FROM %s AS rv "
      "WHERE rv.creative_instance_id = '%s'",
      GetTableName().c_str(), creative_instance_id.c_str());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->command = query;

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE   // expire_at
  };

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnGetForCreativeInstanceId, creative_instance_id,
                     std::move(callback)));
}

void Deposits::PurgeExpired(ResultCallback callback) const {
  const std::string query = base::StringPrintf(
      "DELETE FROM %s "
      "WHERE DATETIME('now') >= DATETIME(expire_at, 'unixepoch')",
      GetTableName().c_str());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->command = query;

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  transaction->commands.push_back(std::move(command));

  AdsClientHelper::GetInstance()->RunDBTransaction(
      std::move(transaction),
      base::BindOnce(&OnResultCallback, std::move(callback)));
}

std::string Deposits::GetTableName() const {
  return kTableName;
}

void Deposits::Migrate(mojom::DBTransactionInfo* transaction,
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
    mojom::DBCommandInfo* command,
    const CreativeAdList& creative_ads) const {
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

std::string Deposits::BuildInsertOrUpdateQuery(
    mojom::DBCommandInfo* command,
    const DepositInfo& deposit) const {
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

}  // namespace ads::database::table
