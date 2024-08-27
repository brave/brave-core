/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_TABLE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_TABLE_UTIL_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads::database {

void CreateTableIndex(mojom::DBTransactionInfo* mojom_db_transaction,
                      const std::string& table_name,
                      const std::vector<std::string>& columns);

void DropTableIndex(mojom::DBTransactionInfo* mojom_db_transaction,
                    const std::string& index_name);

void DropTable(mojom::DBTransactionInfo* mojom_db_transaction,
               const std::string& table_name);

void DeleteTable(mojom::DBTransactionInfo* mojom_db_transaction,
                 const std::string& table_name);

void CopyTableColumns(mojom::DBTransactionInfo* mojom_db_transaction,
                      const std::string& from,
                      const std::string& to,
                      const std::vector<std::string>& from_columns,
                      const std::vector<std::string>& to_columns,
                      bool should_drop);

void CopyTableColumns(mojom::DBTransactionInfo* mojom_db_transaction,
                      const std::string& from,
                      const std::string& to,
                      const std::vector<std::string>& columns,
                      bool should_drop);

void RenameTable(mojom::DBTransactionInfo* mojom_db_transaction,
                 const std::string& from,
                 const std::string& to);

}  // namespace brave_ads::database

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_DATABASE_DATABASE_TABLE_UTIL_H_
