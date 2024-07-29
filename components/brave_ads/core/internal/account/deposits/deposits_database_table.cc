/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_table.h"

#include <cstddef>
#include <utility>

#include "base/check.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_bind_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "deposits";

void BindRecords(mojom::DBCommandInfo* const command) {
  CHECK(command);

  command->record_bindings = {
      mojom::DBCommandInfo::RecordBindingType::
          STRING_TYPE,  // creative_instance_id
      mojom::DBCommandInfo::RecordBindingType::DOUBLE_TYPE,  // value
      mojom::DBCommandInfo::RecordBindingType::INT64_TYPE    // expire_at
  };
}

size_t BindParameters(mojom::DBCommandInfo* command,
                      const CreativeAdList& creative_ads) {
  CHECK(command);

  size_t count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindString(command, index++, creative_ad.creative_instance_id);
    BindDouble(command, index++, creative_ad.value);
    BindInt64(command, index++,
              ToChromeTimestampFromTime(creative_ad.end_at + base::Days(7)));

    ++count;
  }

  return count;
}

void BindParameters(mojom::DBCommandInfo* const command,
                    const DepositInfo& deposit) {
  CHECK(command);
  CHECK(deposit.IsValid());

  BindString(command, 0, deposit.creative_instance_id);
  BindDouble(command, 1, deposit.value);
  BindInt64(
      command, 2,
      ToChromeTimestampFromTime(deposit.expire_at.value_or(base::Time())));
}

DepositInfo GetFromRecord(mojom::DBRecordInfo* const record) {
  CHECK(record);

  DepositInfo deposit;

  deposit.creative_instance_id = ColumnString(record, 0);
  deposit.value = ColumnDouble(record, 1);
  const base::Time expire_at =
      ToTimeFromChromeTimestamp(ColumnInt64(record, 2));
  if (!expire_at.is_null()) {
    deposit.expire_at = expire_at;
  }

  return deposit;
}

void GetForCreativeInstanceIdCallback(
    const std::string& /*creative_instance_id*/,
    GetDepositsCallback callback,
    mojom::DBCommandResponseInfoPtr command_response) {
  if (!command_response ||
      command_response->status !=
          mojom::DBCommandResponseInfo::StatusType::RESPONSE_OK) {
    BLOG(0, "Failed to get deposit value");

    return std::move(callback).Run(/*success=*/false,
                                   /*deposit=*/std::nullopt);
  }

  CHECK(command_response->result);

  if (command_response->result->get_records().empty()) {
    return std::move(callback).Run(/*success=*/true, /*deposit=*/std::nullopt);
  }

  const mojom::DBRecordInfoPtr record =
      std::move(command_response->result->get_records().front());
  DepositInfo deposit = GetFromRecord(&*record);
  if (!deposit.IsValid()) {
    // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
    // potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                              "Invalid deposit");
    base::debug::DumpWithoutCrashing();

    BLOG(0, "Invalid deposit");

    return std::move(callback).Run(/*success=*/false, /*deposit=*/std::nullopt);
  }

  std::move(callback).Run(/*success=*/true, std::move(deposit));
}

void MigrateToV24(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Recreate table to address a migration problem from older versions.
  DropTable(transaction, "deposits");

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          CREATE TABLE deposits (
            creative_instance_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
            value DOUBLE NOT NULL,
            expire_at TIMESTAMP NOT NULL
          );)";
  transaction->commands.push_back(std::move(command));
}

void MigrateToV29(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  // Migrate `expire_at` column from a UNIX timestamp to a WebKit/Chrome
  // timestamp.
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          UPDATE
            deposits
          SET
            expire_at = (
              CAST(expire_at AS INT64) + 11644473600
            ) * 1000000;)";
  transaction->commands.push_back(std::move(command));
}

}  // namespace

void Deposits::Save(const DepositInfo& deposit, ResultCallback callback) {
  if (!deposit.IsValid()) {
    // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
    // potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                              "Invalid deposit");
    base::debug::DumpWithoutCrashing();

    BLOG(0, "Invalid deposit");

    return std::move(callback).Run(/*success=*/false);
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();

  InsertOrUpdate(&*transaction, deposit);

  RunTransaction(std::move(transaction), std::move(callback));
}

void Deposits::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                              const CreativeAdList& creative_ads) {
  CHECK(transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = BuildInsertOrUpdateSql(&*command, creative_ads);
  transaction->commands.push_back(std::move(command));
}

void Deposits::InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                              const DepositInfo& deposit) {
  CHECK(transaction);
  CHECK(deposit.IsValid());

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::RUN;
  command->sql = BuildInsertOrUpdateSql(&*command, deposit);
  transaction->commands.push_back(std::move(command));
}

void Deposits::GetForCreativeInstanceId(const std::string& creative_instance_id,
                                        GetDepositsCallback callback) const {
  if (creative_instance_id.empty()) {
    return std::move(callback).Run(/*success=*/false,
                                   /*deposit=*/std::nullopt);
  }

  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::READ;
  command->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_instance_id,
            value,
            expire_at
          FROM
            $1
          WHERE
            creative_instance_id = '$2';)",
      {GetTableName(), creative_instance_id}, nullptr);
  BindRecords(&*command);
  transaction->commands.push_back(std::move(command));

  RunDBTransaction(std::move(transaction),
                   base::BindOnce(&GetForCreativeInstanceIdCallback,
                                  creative_instance_id, std::move(callback)));
}

void Deposits::PurgeExpired(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr transaction = mojom::DBTransactionInfo::New();
  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql = base::ReplaceStringPlaceholders(
      R"(
          DELETE FROM
            $1
          WHERE
            $2 >= expire_at;)",
      {GetTableName(),
       base::NumberToString(ToChromeTimestampFromTime(base::Time::Now()))},
      nullptr);
  transaction->commands.push_back(std::move(command));

  RunTransaction(std::move(transaction), std::move(callback));
}

std::string Deposits::GetTableName() const {
  return kTableName;
}

void Deposits::Create(mojom::DBTransactionInfo* const transaction) {
  CHECK(transaction);

  mojom::DBCommandInfoPtr command = mojom::DBCommandInfo::New();
  command->type = mojom::DBCommandInfo::Type::EXECUTE;
  command->sql =
      R"(
          CREATE TABLE deposits (
            creative_instance_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
            value DOUBLE NOT NULL,
            expire_at TIMESTAMP NOT NULL
          );)";
  transaction->commands.push_back(std::move(command));
}

void Deposits::Migrate(mojom::DBTransactionInfo* transaction,
                       const int to_version) {
  CHECK(transaction);

  switch (to_version) {
    case 24: {
      MigrateToV24(transaction);
      break;
    }

    case 29: {
      MigrateToV29(transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

std::string Deposits::BuildInsertOrUpdateSql(
    mojom::DBCommandInfo* command,
    const CreativeAdList& creative_ads) const {
  CHECK(command);

  const size_t binded_parameters_count = BindParameters(command, creative_ads);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_instance_id,
            value,
            expire_at
          ) VALUES $2;)",
      {GetTableName(), BuildBindingParameterPlaceholders(
                           /*parameters_count=*/3, binded_parameters_count)},
      nullptr);
}

std::string Deposits::BuildInsertOrUpdateSql(mojom::DBCommandInfo* command,
                                             const DepositInfo& deposit) const {
  CHECK(command);
  CHECK(deposit.IsValid());

  BindParameters(command, deposit);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_instance_id,
            value,
            expire_at
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindingParameterPlaceholders(/*parameters_count=*/3,
                                         /*binded_parameters_count=*/1)},
      nullptr);
}

}  // namespace brave_ads::database::table
