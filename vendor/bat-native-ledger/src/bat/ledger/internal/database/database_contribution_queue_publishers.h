/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_QUEUE_PUBLISHERS_H_
#define BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_QUEUE_PUBLISHERS_H_

#include <string>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

class DatabaseContributionQueuePublishers: public DatabaseTable {
 public:
  explicit DatabaseContributionQueuePublishers(LedgerImpl* ledger);
  ~DatabaseContributionQueuePublishers() override;

  void InsertOrUpdate(
      const std::string& id,
      type::ContributionQueuePublisherList list,
      ledger::ResultCallback callback);

  void GetRecordsByQueueId(
      const std::string& queue_id,
      ContributionQueuePublishersListCallback callback);

 private:
  void OnGetRecordsByQueueId(
      type::DBCommandResponsePtr response,
      ContributionQueuePublishersListCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_QUEUE_PUBLISHERS_H_
