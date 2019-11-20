/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_CONTRIBUTION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_CONTRIBUTION_INFO_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/contribution_info.h"
#include "brave/components/brave_rewards/browser/database/database_table.h"

namespace brave_rewards {

class DatabaseContributionInfo: public DatabaseTable {
 public:
  explicit DatabaseContributionInfo(int current_db_version);
  ~DatabaseContributionInfo() override;

  bool Init(sql::Database* db) override;

  bool CreateTable(sql::Database* db) override;

  bool CreateIndex(sql::Database* db) override;

  bool Migrate(sql::Database* db, const int to);

  bool InsertOrUpdate(
      sql::Database* db,
      const brave_rewards::ContributionInfo& info);

  bool GetOneTimeTips(
      sql::Database* db,
      ledger::PublisherInfoList* list,
      const ledger::ActivityMonth month,
      const int year);

 private:
  const char* table_name_ = "contribution_info";
  const int minimum_version_ = 2;

  bool CreateTableV8(sql::Database* db);

  bool CreateIndexV8(sql::Database* db);

  bool MigrateToV8(sql::Database* db);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_CONTRIBUTION_INFO_H_
