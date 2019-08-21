/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_TABLE_H_

#include "sql/database.h"

namespace brave_rewards {

class DatabaseTable {
 public:
  explicit DatabaseTable();
  virtual ~DatabaseTable();

  virtual bool CreateTable(sql::Database* db) = 0;

  virtual bool CreateIndex(sql::Database* db) = 0;

  bool InsertIndex(
    sql::Database* db,
    const std::string& table_name,
    const std::string& key);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_TABLE_H_