/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_
#define BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_

#include <memory>
#include <string>

#include "bat/ledger/internal/database/database_contribution_queue_publishers.h"
#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

class DatabaseContributionQueue: public DatabaseTable {
 public:
  explicit DatabaseContributionQueue(bat_ledger::LedgerImpl* ledger);
  ~DatabaseContributionQueue() override;

  bool Migrate(ledger::DBTransaction* transaction, const int target) override;

  void InsertOrUpdate(
      ledger::ContributionQueuePtr info,
      ledger::ResultCallback callback);

  void GetFirstRecord(ledger::GetFirstContributionQueueCallback callback);

  void DeleteRecord(const uint64_t id, ledger::ResultCallback callback);

 private:
  bool CreateTableV9(ledger::DBTransaction* transaction);

  bool MigrateToV9(ledger::DBTransaction* transaction);

  bool MigrateToV15(ledger::DBTransaction* transaction);

  void OnInsertOrUpdate(
      ledger::DBCommandResponsePtr response,
      const std::string& queue_string,
      const uint64_t id,
      ledger::ResultCallback callback);

  void OnGetFirstRecord(
      ledger::DBCommandResponsePtr response,
      ledger::GetFirstContributionQueueCallback callback);

  void OnGetPublishers(
      ledger::ContributionQueuePublisherList list,
      const std::string& queue_string,
      ledger::GetFirstContributionQueueCallback callback);

  std::unique_ptr<DatabaseContributionQueuePublishers> publishers_;
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_
