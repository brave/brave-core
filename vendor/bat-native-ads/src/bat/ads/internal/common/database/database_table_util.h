/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_DATABASE_DATABASE_TABLE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_DATABASE_DATABASE_TABLE_UTIL_H_

#include <string>
#include <vector>

#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace ads::database {

void CreateTableIndex(mojom::DBTransactionInfo* transaction,
                      const std::string& table_name,
                      const std::string& key);

void DropTable(mojom::DBTransactionInfo* transaction,
               const std::string& table_name);

void DeleteTable(mojom::DBTransactionInfo* transaction,
                 const std::string& table_name);

void CopyTableColumns(mojom::DBTransactionInfo* transaction,
                      const std::string& from,
                      const std::string& to,
                      const std::vector<std::string>& from_columns,
                      const std::vector<std::string>& to_columns,
                      bool should_drop,
                      const std::string& group_by = "");

void CopyTableColumns(mojom::DBTransactionInfo* transaction,
                      const std::string& from,
                      const std::string& to,
                      const std::vector<std::string>& columns,
                      bool should_drop,
                      const std::string& group_by = "");

void RenameTable(mojom::DBTransactionInfo* transaction,
                 const std::string& from,
                 const std::string& to);

}  // namespace ads::database

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COMMON_DATABASE_DATABASE_TABLE_UTIL_H_
