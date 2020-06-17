/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_DATABASE_DATABASE_TABLE_UTIL_H_
#define BAT_ADS_INTERNAL_DATABASE_DATABASE_TABLE_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ads/mojom.h"

namespace ads {
namespace database {
namespace table {

void Drop(
    DBTransaction* transaction,
    const std::string& table_name);

void Delete(
    DBTransaction* transaction,
    const std::string& table_name);

std::string BuildInsertQuery(
    const std::string& from,
    const std::string& to,
    const std::map<std::string, std::string>& columns,
    const std::string& group_by);

void Migrate(
    DBTransaction* transaction,
    const std::string& from,
    const std::string& to,
    const std::map<std::string, std::string>& columns,
    const bool should_drop,
    const std::string& group_by = "");

void Migrate(
    DBTransaction* transaction,
    const std::string& from,
    const std::string& to,
    const std::vector<std::string>& columns,
    const bool should_drop,
    const std::string& group_by = "");

void Rename(
    DBTransaction* transaction,
    const std::string& from,
    const std::string& to);

void CreateIndex(
    DBTransaction* transaction,
    const std::string& table_name,
    const std::string& key);

}  // namespace table
}  // namespace database
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_DATABASE_DATABASE_TABLE_UTIL_H_
