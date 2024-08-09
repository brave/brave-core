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
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "deposits";

void BindColumnTypes(mojom::DBStatementInfo* const mojom_statement) {
  CHECK(mojom_statement);

  mojom_statement->bind_column_types = {
      mojom::DBBindColumnType::kString,  // creative_instance_id
      mojom::DBBindColumnType::kDouble,  // value
      mojom::DBBindColumnType::kInt64    // expire_at
  };
}

size_t BindColumns(mojom::DBStatementInfo* mojom_statement,
                   const CreativeAdList& creative_ads) {
  CHECK(mojom_statement);
  CHECK(!creative_ads.empty());

  size_t row_count = 0;

  int index = 0;
  for (const auto& creative_ad : creative_ads) {
    BindColumnString(mojom_statement, index++,
                     creative_ad.creative_instance_id);
    BindColumnDouble(mojom_statement, index++, creative_ad.value);
    BindColumnInt64(
        mojom_statement, index++,
        ToChromeTimestampFromTime(creative_ad.end_at + base::Days(7)));

    ++row_count;
  }

  return row_count;
}

void BindColumns(mojom::DBStatementInfo* const mojom_statement,
                 const DepositInfo& deposit) {
  CHECK(mojom_statement);
  CHECK(deposit.IsValid());

  BindColumnString(mojom_statement, 0, deposit.creative_instance_id);
  BindColumnDouble(mojom_statement, 1, deposit.value);
  BindColumnInt64(
      mojom_statement, 2,
      ToChromeTimestampFromTime(deposit.expire_at.value_or(base::Time())));
}

DepositInfo FromMojomRow(const mojom::DBRowInfo* const mojom_row) {
  CHECK(mojom_row);

  DepositInfo deposit;

  deposit.creative_instance_id = ColumnString(mojom_row, 0);
  deposit.value = ColumnDouble(mojom_row, 1);
  const base::Time expire_at =
      ToTimeFromChromeTimestamp(ColumnInt64(mojom_row, 2));
  if (!expire_at.is_null()) {
    deposit.expire_at = expire_at;
  }

  return deposit;
}

void GetForCreativeInstanceIdCallback(
    const std::string& /*creative_instance_id*/,
    GetDepositsCallback callback,
    mojom::DBStatementResultInfoPtr mojom_statement_result) {
  if (!mojom_statement_result ||
      mojom_statement_result->result_code !=
          mojom::DBStatementResultInfo::ResultCode::kSuccess) {
    BLOG(0, "Failed to get deposit value");

    return std::move(callback).Run(/*success=*/false,
                                   /*deposit=*/std::nullopt);
  }

  CHECK(mojom_statement_result->rows_union);

  if (mojom_statement_result->rows_union->get_rows().empty()) {
    return std::move(callback).Run(/*success=*/true, /*deposit=*/std::nullopt);
  }

  const mojom::DBRowInfoPtr mojom_row =
      std::move(mojom_statement_result->rows_union->get_rows().front());
  DepositInfo deposit = FromMojomRow(&*mojom_row);
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

void MigrateToV24(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Recreate table to address a migration problem from older versions.
  DropTable(mojom_transaction, "deposits");

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kExecute;
  mojom_statement->sql =
      R"(
          CREATE TABLE deposits (
            creative_instance_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
            value DOUBLE NOT NULL,
            expire_at TIMESTAMP NOT NULL
          );)";
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

void MigrateToV29(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Migrate `expire_at` column from a UNIX timestamp to a WebKit/Chrome
  // timestamp.
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kExecute;
  mojom_statement->sql =
      R"(
          UPDATE
            deposits
          SET
            expire_at = (
              CAST(expire_at AS INT64) + 11644473600
            ) * 1000000;)";
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

void MigrateToV43(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  // Optimize database query for `GetForCreativeInstanceId`.
  CreateTableIndex(mojom_transaction, /*table_name=*/"deposits",
                   /*columns=*/{"creative_instance_id"});

  // Optimize database query for `PurgeExpired`.
  CreateTableIndex(mojom_transaction, /*table_name=*/"deposits",
                   /*columns=*/{"expire_at"});
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

  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();

  Insert(&*mojom_transaction, deposit);

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

void Deposits::Insert(mojom::DBTransactionInfo* mojom_transaction,
                      const CreativeAdList& creative_ads) {
  CHECK(mojom_transaction);

  if (creative_ads.empty()) {
    return;
  }

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type = mojom::DBStatementInfo::OperationType::kRun;
  mojom_statement->sql = BuildInsertSql(&*mojom_statement, creative_ads);
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

void Deposits::Insert(mojom::DBTransactionInfo* mojom_transaction,
                      const DepositInfo& deposit) {
  CHECK(mojom_transaction);
  CHECK(deposit.IsValid());

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type = mojom::DBStatementInfo::OperationType::kRun;
  mojom_statement->sql = BuildInsertSql(&*mojom_statement, deposit);
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

void Deposits::GetForCreativeInstanceId(const std::string& creative_instance_id,
                                        GetDepositsCallback callback) const {
  if (creative_instance_id.empty()) {
    return std::move(callback).Run(/*success=*/false,
                                   /*deposit=*/std::nullopt);
  }

  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kStep;
  mojom_statement->sql = base::ReplaceStringPlaceholders(
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
  BindColumnTypes(&*mojom_statement);
  mojom_transaction->statements.push_back(std::move(mojom_statement));

  RunDBTransaction(std::move(mojom_transaction),
                   base::BindOnce(&GetForCreativeInstanceIdCallback,
                                  creative_instance_id, std::move(callback)));
}

void Deposits::PurgeExpired(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kExecute;
  mojom_statement->sql = base::ReplaceStringPlaceholders(
      R"(
          DELETE FROM
            $1
          WHERE
            $2 >= expire_at;)",
      {GetTableName(),
       base::NumberToString(ToChromeTimestampFromTime(base::Time::Now()))},
      nullptr);
  mojom_transaction->statements.push_back(std::move(mojom_statement));

  RunTransaction(std::move(mojom_transaction), std::move(callback));
}

std::string Deposits::GetTableName() const {
  return kTableName;
}

void Deposits::Create(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kExecute;
  mojom_statement->sql =
      R"(
          CREATE TABLE deposits (
            creative_instance_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
            value DOUBLE NOT NULL,
            expire_at TIMESTAMP NOT NULL
          );)";
  mojom_transaction->statements.push_back(std::move(mojom_statement));

  // Optimize database query for `GetForCreativeInstanceId` from schema 43.
  CreateTableIndex(mojom_transaction, GetTableName(),
                   /*columns=*/{"creative_instance_id"});

  // Optimize database query for `PurgeExpired` from schema 43.
  CreateTableIndex(mojom_transaction, GetTableName(),
                   /*columns=*/{"expire_at"});
}

void Deposits::Migrate(mojom::DBTransactionInfo* mojom_transaction,
                       const int to_version) {
  CHECK(mojom_transaction);

  switch (to_version) {
    case 24: {
      MigrateToV24(mojom_transaction);
      break;
    }

    case 29: {
      MigrateToV29(mojom_transaction);
      break;
    }

    case 43: {
      MigrateToV43(mojom_transaction);
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

std::string Deposits::BuildInsertSql(mojom::DBStatementInfo* mojom_statement,
                                     const CreativeAdList& creative_ads) const {
  CHECK(mojom_statement);
  CHECK(!creative_ads.empty());

  const size_t row_count = BindColumns(mojom_statement, creative_ads);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_instance_id,
            value,
            expire_at
          ) VALUES $2;)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/3, row_count)},
      nullptr);
}

std::string Deposits::BuildInsertSql(mojom::DBStatementInfo* mojom_statement,
                                     const DepositInfo& deposit) const {
  CHECK(mojom_statement);
  CHECK(deposit.IsValid());

  BindColumns(mojom_statement, deposit);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_instance_id,
            value,
            expire_at
          ) VALUES $2;)",
      {GetTableName(), BuildBindColumnPlaceholder(/*column_count=*/3)},
      nullptr);
}

}  // namespace brave_ads::database::table
