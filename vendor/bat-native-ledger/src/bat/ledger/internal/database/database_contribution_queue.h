/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_contribution_queue_publishers.h"
#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

using GetFirstContributionQueueCallback =
    std::function<void(mojom::ContributionQueuePtr)>;

class DatabaseContributionQueue: public DatabaseTable {
 public:
  explicit DatabaseContributionQueue(LedgerImpl* ledger);
  ~DatabaseContributionQueue() override;

  void InsertOrUpdate(mojom::ContributionQueuePtr info,
                      ledger::LegacyResultCallback callback);

  void GetFirstRecord(GetFirstContributionQueueCallback callback);

  void MarkRecordAsComplete(const std::string& id,
                            ledger::LegacyResultCallback callback);

 private:
  void OnInsertOrUpdate(
      mojom::DBCommandResponsePtr response,
      std::shared_ptr<mojom::ContributionQueuePtr> shared_queue,
      ledger::LegacyResultCallback callback);

  void OnGetFirstRecord(mojom::DBCommandResponsePtr response,
                        GetFirstContributionQueueCallback callback);

  void OnGetPublishers(
      std::vector<mojom::ContributionQueuePublisherPtr> list,
      std::shared_ptr<mojom::ContributionQueuePtr> shared_queue,
      GetFirstContributionQueueCallback callback);

  std::unique_ptr<DatabaseContributionQueuePublishers> publishers_;
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_
