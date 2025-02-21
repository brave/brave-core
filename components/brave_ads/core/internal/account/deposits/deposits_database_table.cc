/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_table.h"

#include <cstddef>
#include <cstdint>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposit_info.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_deposit_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "deposits";

void BindColumnTypes(const mojom::DBActionInfoPtr& mojom_db_action) {
  CHECK(mojom_db_action);

  mojom_db_action->bind_column_types = {
      mojom::DBBindColumnType::kString,  // creative_instance_id
      mojom::DBBindColumnType::kDouble,  // value
      mojom::DBBindColumnType::kTime     // expire_at
  };
}

size_t BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                   const std::map</*creative_instance_id*/ std::string,
                                  CreativeDepositInfo>& deposits) {
  CHECK(mojom_db_action);
  CHECK(!deposits.empty());

  size_t row_count = 0;

  int32_t index = 0;
  for (const auto& [creative_instance_id, deposit] : deposits) {
    BindColumnString(mojom_db_action, index++, creative_instance_id);
    BindColumnDouble(mojom_db_action, index++, deposit.value);
    BindColumnTime(mojom_db_action, index++, deposit.expire_at);

    ++row_count;
  }

  return row_count;
}

void BindColumns(const mojom::DBActionInfoPtr& mojom_db_action,
                 const DepositInfo& deposit) {
  CHECK(mojom_db_action);
  CHECK(deposit.IsValid());

  BindColumnString(mojom_db_action, 0, deposit.creative_instance_id);
  BindColumnDouble(mojom_db_action, 1, deposit.value);
  BindColumnTime(mojom_db_action, 2, deposit.expire_at.value_or(base::Time()));
}

DepositInfo FromMojomRow(const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);

  DepositInfo deposit;

  deposit.creative_instance_id = ColumnString(mojom_db_row, 0);
  deposit.value = ColumnDouble(mojom_db_row, 1);
  const base::Time expire_at = ColumnTime(mojom_db_row, 2);
  if (!expire_at.is_null()) {
    deposit.expire_at = expire_at;
  }

  return deposit;
}

void GetForCreativeInstanceIdCallback(
    const std::string& /*creative_instance_id*/,
    GetDepositsCallback callback,
    mojom::DBTransactionResultInfoPtr mojom_db_transaction_result) {
  if (!IsTransactionSuccessful(mojom_db_transaction_result)) {
    BLOG(0, "Failed to get deposit value");
    return std::move(callback).Run(/*success=*/false,
                                   /*deposit=*/std::nullopt);
  }

  CHECK(mojom_db_transaction_result->rows_union);

  if (mojom_db_transaction_result->rows_union->get_rows().empty()) {
    return std::move(callback).Run(/*success=*/true, /*deposit=*/std::nullopt);
  }

  const mojom::DBRowInfoPtr mojom_db_row =
      std::move(mojom_db_transaction_result->rows_union->get_rows().front());
  DepositInfo deposit = FromMojomRow(mojom_db_row);
  if (!deposit.IsValid()) {
    BLOG(0, "Invalid deposit");
    return std::move(callback).Run(/*success=*/false, /*deposit=*/std::nullopt);
  }

  std::move(callback).Run(/*success=*/true, std::move(deposit));
}

void MigrateToV43(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  // Optimize database query for `GetForCreativeInstanceId`.
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"deposits",
                   /*columns=*/{"creative_instance_id"});

  // Optimize database query for `PurgeExpired`.
  CreateTableIndex(mojom_db_transaction, /*table_name=*/"deposits",
                   /*columns=*/{"expire_at"});
}

}  // namespace

void Deposits::Save(const DepositInfo& deposit, ResultCallback callback) {
  if (!deposit.IsValid()) {
    BLOG(0, "Invalid deposit");
    return std::move(callback).Run(/*success=*/false);
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();

  Insert(mojom_db_transaction, deposit);

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

void Deposits::Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                      const std::map</*creative_instance_id*/ std::string,
                                     CreativeDepositInfo>& deposits) {
  CHECK(mojom_db_transaction);

  if (deposits.empty()) {
    return;
  }

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, deposits);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

void Deposits::Insert(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                      const DepositInfo& deposit) {
  CHECK(mojom_db_transaction);
  CHECK(deposit.IsValid());

  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteWithBindings;
  mojom_db_action->sql = BuildInsertSql(mojom_db_action, deposit);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));
}

void Deposits::GetForCreativeInstanceId(const std::string& creative_instance_id,
                                        GetDepositsCallback callback) const {
  if (creative_instance_id.empty()) {
    return std::move(callback).Run(/*success=*/false,
                                   /*deposit=*/std::nullopt);
  }

  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  mojom::DBActionInfoPtr mojom_db_action = mojom::DBActionInfo::New();
  mojom_db_action->type = mojom::DBActionInfo::Type::kExecuteQueryWithBindings;
  mojom_db_action->sql = base::ReplaceStringPlaceholders(
      R"(
          SELECT
            creative_instance_id,
            value,
            expire_at
          FROM
            $1
          WHERE
            creative_instance_id = '$2')",
      {GetTableName(), creative_instance_id}, nullptr);
  BindColumnTypes(mojom_db_action);
  mojom_db_transaction->actions.push_back(std::move(mojom_db_action));

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 base::BindOnce(&GetForCreativeInstanceIdCallback,
                                creative_instance_id, std::move(callback)));
}

void Deposits::PurgeExpired(ResultCallback callback) const {
  mojom::DBTransactionInfoPtr mojom_db_transaction =
      mojom::DBTransactionInfo::New();
  Execute(mojom_db_transaction, R"(
            DELETE FROM
              $1
            WHERE
              $2 >= expire_at)",
          {GetTableName(), TimeToSqlValueAsString(base::Time::Now())});

  RunTransaction(FROM_HERE, std::move(mojom_db_transaction),
                 std::move(callback));
}

std::string Deposits::GetTableName() const {
  return kTableName;
}

void Deposits::Create(const mojom::DBTransactionInfoPtr& mojom_db_transaction) {
  CHECK(mojom_db_transaction);

  Execute(mojom_db_transaction, R"(
      CREATE TABLE deposits (
        creative_instance_id TEXT NOT NULL PRIMARY KEY ON CONFLICT REPLACE,
        value DOUBLE NOT NULL,
        expire_at TIMESTAMP NOT NULL
      ))");

  // Optimize database query for `GetForCreativeInstanceId` from schema 43.
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"creative_instance_id"});

  // Optimize database query for `PurgeExpired` from schema 43.
  CreateTableIndex(mojom_db_transaction, GetTableName(),
                   /*columns=*/{"expire_at"});
}

void Deposits::Migrate(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                       int to_version) {
  CHECK(mojom_db_transaction);

  switch (to_version) {
    case 43: {
      MigrateToV43(mojom_db_transaction);
      break;
    }

    default: {
      // No migration needed.
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

std::string Deposits::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const std::map</*creative_instance_id*/ std::string, CreativeDepositInfo>&
        deposits) const {
  CHECK(mojom_db_action);
  CHECK(!deposits.empty());

  const size_t row_count = BindColumns(mojom_db_action, deposits);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_instance_id,
            value,
            expire_at
          ) VALUES $2)",
      {GetTableName(),
       BuildBindColumnPlaceholders(/*column_count=*/3, row_count)},
      nullptr);
}

std::string Deposits::BuildInsertSql(
    const mojom::DBActionInfoPtr& mojom_db_action,
    const DepositInfo& deposit) const {
  CHECK(mojom_db_action);
  CHECK(deposit.IsValid());

  BindColumns(mojom_db_action, deposit);

  return base::ReplaceStringPlaceholders(
      R"(
          INSERT INTO $1 (
            creative_instance_id,
            value,
            expire_at
          ) VALUES $2)",
      {GetTableName(), BuildBindColumnPlaceholder(/*column_count=*/3)},
      nullptr);
}

}  // namespace brave_ads::database::table
