/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/database/database_statement_util.h"

#include <utility>

#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "sql/statement.h"

namespace brave_ads::database {

void Execute(mojom::DBTransactionInfo* const mojom_transaction,
             const std::string& sql) {
  CHECK(mojom_transaction);

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kExecute;
  mojom_statement->sql = sql;
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

void Execute(mojom::DBTransactionInfo* const mojom_transaction,
             const std::string& sql,
             const std::vector<std::string>& subst) {
  CHECK(mojom_transaction);

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kExecute;
  mojom_statement->sql = base::ReplaceStringPlaceholders(sql, subst, nullptr);
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

void Vacuum(mojom::DBTransactionInfo* const mojom_transaction) {
  CHECK(mojom_transaction);

  mojom::DBStatementInfoPtr mojom_statement = mojom::DBStatementInfo::New();
  mojom_statement->operation_type =
      mojom::DBStatementInfo::OperationType::kVacuum;
  mojom_transaction->statements.push_back(std::move(mojom_statement));
}

std::string TimeToSqlValueAsString(base::Time time) {
  return base::NumberToString(sql::Statement::TimeToSqlValue(time));
}

}  // namespace brave_ads::database
