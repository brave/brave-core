/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_CONTRIBUTION_QUEUE_PUBLISHERS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_CONTRIBUTION_QUEUE_PUBLISHERS_H_

#include <stdint.h>
#include <string>

#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/database/database_table.h"

namespace brave_rewards {

class DatabaseContributionQueuePublishers: public DatabaseTable {
 public:
  explicit DatabaseContributionQueuePublishers(int current_db_version);
  ~DatabaseContributionQueuePublishers() override;

  bool Init(sql::Database* db) override;

  bool CreateTable(sql::Database* db) override;

  bool CreateIndex(sql::Database* db) override;

  bool InsertOrUpdate(
      sql::Database* db,
      ledger::ContributionQueuePtr info);

  ledger::ContributionQueuePublisherList GetRecords(
      sql::Database* db,
      const uint64_t queue_id);

 private:
  const char* table_name_ = "contribution_queue_publishers";
  const int minimum_version_ = 9;
  const char* parent_table_name_ = "contribution_queue";
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_CONTRIBUTION_QUEUE_PUBLISHERS_H_
