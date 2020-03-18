/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_INFO_PUBLISHERS_H_
#define BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_INFO_PUBLISHERS_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

class DatabaseContributionInfoPublishers: public DatabaseTable {
 public:
  explicit DatabaseContributionInfoPublishers(bat_ledger::LedgerImpl* ledger);
  ~DatabaseContributionInfoPublishers() override;

  bool Migrate(ledger::DBTransaction* transaction, const int target) override;

  void InsertOrUpdate(
      ledger::DBTransaction* transaction,
      ledger::ContributionInfoPtr info);

  void GetRecordByContributionList(
      const std::vector<std::string>& contribution_ids,
      ContributionPublisherListCallback callback);

  void GetContributionPublisherPairList(
      const std::vector<std::string>& contribution_ids,
      ContributionPublisherPairListCallback callback);

  void UpdateContributedAmount(
      const std::string& contribution_id,
      const std::string& publisher_key,
      ledger::ResultCallback callback);

 private:
  bool CreateTableV11(ledger::DBTransaction* transaction);

  bool CreateTableV15(ledger::DBTransaction* transaction);

  bool CreateIndexV11(ledger::DBTransaction* transaction);

  bool CreateIndexV15(ledger::DBTransaction* transaction);

  bool MigrateToV11(ledger::DBTransaction* transaction);

  bool MigrateToV15(ledger::DBTransaction* transaction);

  void OnGetRecordByContributionList(
      ledger::DBCommandResponsePtr response,
      ContributionPublisherListCallback callback);

  void OnGetContributionPublisherInfoMap(
      ledger::DBCommandResponsePtr response,
      ContributionPublisherPairListCallback callback);
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_INFO_PUBLISHERS_H_
