/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/database/database_table_util.h"

#include "base/check_op.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/internal/common/database/database_transaction_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads::database {

void CreateTableIndex(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                      const std::string& table_name,
                      const std::vector<std::string>& columns) {
  CHECK(mojom_db_transaction);
  CHECK(!table_name.empty());
  CHECK(!columns.empty());

  Execute(mojom_db_transaction, R"(
            CREATE INDEX IF NOT EXISTS
              $1_$2_index ON $3 ($4);)",
          {table_name, base::JoinString(columns, "_"), table_name,
           base::JoinString(columns, ", ")});
}

void DropTableIndex(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                    const std::string& index_name) {
  CHECK(mojom_db_transaction);
  CHECK(!index_name.empty());

  Execute(mojom_db_transaction, R"(
      DROP INDEX IF EXISTS
        ad_events_created_at_index;)");
}

void DropTable(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
               const std::string& table_name) {
  CHECK(mojom_db_transaction);
  CHECK(!table_name.empty());

  Execute(mojom_db_transaction, R"(
            DROP TABLE IF EXISTS
              $1;)",
          {table_name});
}

void DeleteTable(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                 const std::string& table_name) {
  CHECK(mojom_db_transaction);
  CHECK(!table_name.empty());

  Execute(mojom_db_transaction, R"(
            DELETE FROM
              $1;)",
          {table_name});
}

void CopyTableColumns(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                      const std::string& from,
                      const std::string& to,
                      const std::vector<std::string>& from_columns,
                      const std::vector<std::string>& to_columns,
                      const bool should_drop) {
  CHECK(mojom_db_transaction);
  CHECK(!from.empty());
  CHECK(!to.empty());
  CHECK_NE(from, to);
  CHECK(!from_columns.empty());
  CHECK(!to_columns.empty());
  CHECK_EQ(from_columns.size(), to_columns.size());

  Execute(mojom_db_transaction, R"(
            INSERT INTO $1 (
              $2
            )
            SELECT
              $3
            FROM
              $4;)",
          {to, base::JoinString(to_columns, ", "),
           base::JoinString(from_columns, ", "), from});

  if (should_drop) {
    DropTable(mojom_db_transaction, from);
  }
}

void CopyTableColumns(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                      const std::string& from,
                      const std::string& to,
                      const std::vector<std::string>& columns,
                      const bool should_drop) {
  return CopyTableColumns(mojom_db_transaction, from, to, columns, columns,
                          should_drop);
}

void RenameTable(const mojom::DBTransactionInfoPtr& mojom_db_transaction,
                 const std::string& from,
                 const std::string& to) {
  CHECK(mojom_db_transaction);
  CHECK(!from.empty());
  CHECK(!to.empty());
  CHECK_NE(from, to);

  Execute(mojom_db_transaction, R"(
            ALTER TABLE
              $1 RENAME TO $2;)",
          {from, to});
}

}  // namespace brave_ads::database
