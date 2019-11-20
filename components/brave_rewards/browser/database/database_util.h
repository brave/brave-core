/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "sql/database.h"

namespace brave_rewards {

std::string GenerateDBInsertQuery(
  sql::Database* db,
  const std::string& from,
  const std::string& to,
  const std::map<std::string, std::string>& columns);

bool MigrateDBTable(
  sql::Database* db,
  const std::string& from,
  const std::string& to,
  const std::map<std::string, std::string>& columns,
  const bool should_drop);

bool MigrateDBTable(
    sql::Database* db,
    const std::string& from,
    const std::string& to,
    const std::vector<std::string>& columns,
    const bool should_drop);

bool RenameDBTable(
    sql::Database* db,
    const std::string& from,
    const std::string& to);

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_UTIL_H_
