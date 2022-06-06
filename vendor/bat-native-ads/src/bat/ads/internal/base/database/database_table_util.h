/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_DATABASE_DATABASE_TABLE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_DATABASE_DATABASE_TABLE_UTIL_H_

#include <string>
#include <vector>

#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {
namespace database {

void CreateTableIndex(mojom::DBTransaction* transaction,
                      const std::string& table_name,
                      const std::string& key);

void DropTable(mojom::DBTransaction* transaction,
               const std::string& table_name);

void DeleteTable(mojom::DBTransaction* transaction,
                 const std::string& table_name);

void CopyTableColumns(mojom::DBTransaction* transaction,
                      const std::string& from,
                      const std::string& to,
                      const std::vector<std::string>& from_columns,
                      const std::vector<std::string>& to_columns,
                      const bool should_drop,
                      const std::string& group_by = "");

void CopyTableColumns(mojom::DBTransaction* transaction,
                      const std::string& from,
                      const std::string& to,
                      const std::vector<std::string>& columns,
                      const bool should_drop,
                      const std::string& group_by = "");

void RenameTable(mojom::DBTransaction* transaction,
                 const std::string& from,
                 const std::string& to);

}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_DATABASE_DATABASE_TABLE_UTIL_H_
