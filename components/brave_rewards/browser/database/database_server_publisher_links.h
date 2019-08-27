/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_SERVER_PUBLISHER_LINKS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_SERVER_PUBLISHER_LINKS_H_

#include <string>

#include "base/containers/flat_map.h"
#include "bat/ledger/publisher_info.h"
#include "brave/components/brave_rewards/browser/database/database_table.h"

namespace brave_rewards {

class DatabaseServerPublisherLinks: public DatabaseTable {
 public:
  explicit DatabaseServerPublisherLinks(int current_db_version);
  ~DatabaseServerPublisherLinks() override;

  bool Init(sql::Database* db) override;

  bool CreateTable(sql::Database* db) override;

  bool CreateIndex(sql::Database* db) override;

  bool InsertOrUpdate(sql::Database* db, ledger::ServerPublisherInfoPtr info);

  base::flat_map<std::string, std::string> GetRecord(
      sql::Database* db,
      const std::string& publisher_key);

 private:
  const char* table_name_ = "server_publisher_links";
  const int minimum_version_ = 7;
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_DATABASE_DATABASE_SERVER_PUBLISHER_LINKS_H_
