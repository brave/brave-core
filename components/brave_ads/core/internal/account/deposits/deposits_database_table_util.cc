/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_table_util.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposit_info.h"
#include "brave/components/brave_ads/core/internal/common/database/database_column_util.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_deposit_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database::table {

namespace {

constexpr char kTableName[] = "deposits";

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

std::string BuildInsertSql(const mojom::DBActionInfoPtr& mojom_db_action,
                           const std::map</*creative_instance_id*/ std::string,
                                          CreativeDepositInfo>& deposits) {
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
      {kTableName, BuildBindColumnPlaceholders(/*column_count=*/3, row_count)},
      nullptr);
}

}  // namespace

DepositInfo DepositFromMojomRow(const mojom::DBRowInfoPtr& mojom_db_row) {
  CHECK(mojom_db_row);
  CHECK_EQ(3U, mojom_db_row->column_values_union.size());

  DepositInfo deposit;

  deposit.creative_instance_id = ColumnString(mojom_db_row, 0);
  deposit.value = ColumnDouble(mojom_db_row, 1);
  const base::Time expire_at = ColumnTime(mojom_db_row, 2);
  if (!expire_at.is_null()) {
    deposit.expire_at = expire_at;
  }

  return deposit;
}

void InsertDeposits(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
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

}  // namespace brave_ads::database::table
