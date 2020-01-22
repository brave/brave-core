/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_CONTRIBUTION_INFO_PUBLISHERS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_CONTRIBUTION_INFO_PUBLISHERS_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/database/database_table.h"

namespace brave_rewards {

class DatabaseContributionInfoPublishers: public DatabaseTable {
 public:
  explicit DatabaseContributionInfoPublishers(int current_db_version);
  ~DatabaseContributionInfoPublishers() override;

  bool Init(sql::Database* db) override;

  bool CreateTable(sql::Database* db) override;

  bool CreateIndex(sql::Database* db) override;

  bool Migrate(sql::Database* db, const int target);

  bool InsertOrUpdate(
      sql::Database* db,
      ledger::ContributionInfoPtr info);

  bool GetRecords(
    sql::Database* db,
    const std::string& contribution_id,
    ledger::ContributionPublisherList* list);

  bool GetPublisherInfoList(
    sql::Database* db,
    const std::string& contribution_id,
    ledger::PublisherInfoList* list);

  bool UpdateContributedAmount(
      sql::Database* db,
      const std::string& contribution_id,
      const std::string& publisher_key);

 private:
  const char* table_name_ = "contribution_info_publishers";
  const int minimum_version_ = 11;

  bool MigrateToV11(sql::Database* db);

  bool CreateTable11(sql::Database* db);

  bool CreateIndex11(sql::Database* db);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_CONTRIBUTION_INFO_PUBLISHERS_H_
