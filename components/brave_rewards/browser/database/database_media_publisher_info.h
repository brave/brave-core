/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_MEDIA_PUBLISHER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_MEDIA_PUBLISHER_INFO_H_

#include <string>

#include "bat/ledger/mojom_structs.h"
#include "brave/components/brave_rewards/browser/database/database_table.h"

namespace brave_rewards {

class DatabaseMediaPublisherInfo: public DatabaseTable {
 public:
  explicit DatabaseMediaPublisherInfo(int current_db_version);

  ~DatabaseMediaPublisherInfo() override;

  bool Init(sql::Database* db) override;

  bool CreateTable(sql::Database* db) override;

  bool CreateIndex(sql::Database* db) override;

  bool InsertOrUpdate(
      sql::Database* db,
      const std::string& media_key,
      const std::string& publisher_key);

  ledger::PublisherInfoPtr GetRecord(
      sql::Database* db,
      const std::string& media_key);

 private:
  bool CreateTableV1(sql::Database* db);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_MEDIA_PUBLISHER_INFO_H_
