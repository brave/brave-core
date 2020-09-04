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

namespace ledger {
namespace database {

using GetFirstContributionQueueCallback =
    std::function<void(type::ContributionQueuePtr)>;

class DatabaseContributionQueue: public DatabaseTable {
 public:
  explicit DatabaseContributionQueue(LedgerImpl* ledger);
  ~DatabaseContributionQueue() override;

  void InsertOrUpdate(
      type::ContributionQueuePtr info,
      ledger::ResultCallback callback);

  void GetFirstRecord(GetFirstContributionQueueCallback callback);

  void MarkRecordAsComplete(
      const std::string& id,
      ledger::ResultCallback callback);

 private:
  void OnInsertOrUpdate(
      type::DBCommandResponsePtr response,
      std::shared_ptr<type::ContributionQueuePtr> shared_queue,
      ledger::ResultCallback callback);

  void OnGetFirstRecord(
      type::DBCommandResponsePtr response,
      GetFirstContributionQueueCallback callback);

  void OnGetPublishers(
      type::ContributionQueuePublisherList list,
      std::shared_ptr<type::ContributionQueuePtr> shared_queue,
      GetFirstContributionQueueCallback callback);

  std::unique_ptr<DatabaseContributionQueuePublishers> publishers_;
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_
