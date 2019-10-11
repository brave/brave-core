/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_

#include <memory>
#include <string>

#include "bat/ledger/contribution_queue.h"
#include "brave/components/brave_rewards/browser/database/database_contribution_queue_publishers.h"
#include "brave/components/brave_rewards/browser/database/database_table.h"

namespace brave_rewards {

class DatabaseContributionQueue: public DatabaseTable {
 public:
  explicit DatabaseContributionQueue(int current_db_version);
  ~DatabaseContributionQueue() override;

  bool Init(sql::Database* db) override;

  bool CreateTable(sql::Database* db) override;

  bool CreateIndex(sql::Database* db) override;

  bool InsertOrUpdate(sql::Database* db, ledger::ContributionQueuePtr info);

  ledger::ContributionQueuePtr GetFirstRecord(sql::Database* db);

  bool DeleteRecord(sql::Database* db, const uint64_t id);

 private:
  std::string GetIdColumnName();

  const char* table_name_ = "contribution_queue";
  const int minimum_version_ = 9;
  std::unique_ptr<DatabaseContributionQueuePublishers> publishers_;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_
