/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_TABLE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_TABLE_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {
namespace database {
namespace table {
namespace util {

void CreateIndex(mojom::DBTransaction* transaction,
                 const std::string& table_name,
                 const std::string& key);

void Drop(mojom::DBTransaction* transaction, const std::string& table_name);

void Delete(mojom::DBTransaction* transaction, const std::string& table_name);

void CopyColumns(mojom::DBTransaction* transaction,
                 const std::string& from,
                 const std::string& to,
                 const std::map<std::string, std::string>& columns,
                 const bool should_drop,
                 const std::string& group_by = "");

void CopyColumns(mojom::DBTransaction* transaction,
                 const std::string& from,
                 const std::string& to,
                 const std::vector<std::string>& columns,
                 const bool should_drop,
                 const std::string& group_by = "");

void Rename(mojom::DBTransaction* transaction,
            const std::string& from,
            const std::string& to);

}  // namespace util
}  // namespace table
}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DATABASE_DATABASE_TABLE_UTIL_H_
