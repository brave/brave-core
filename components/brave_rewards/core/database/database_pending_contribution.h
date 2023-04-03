/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PENDING_CONTRIBUTION_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PENDING_CONTRIBUTION_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/database/database_table.h"

namespace ledger {
namespace database {

class DatabasePendingContribution : public DatabaseTable {
 public:
  explicit DatabasePendingContribution(LedgerImpl* ledger);
  ~DatabasePendingContribution() override;

  void InsertOrUpdateList(std::vector<mojom::PendingContributionPtr> list,
                          ledger::LegacyResultCallback callback);

  void GetReservedAmount(ledger::GetPendingContributionsTotalCallback callback);

  void GetAllRecords(ledger::GetPendingContributionsCallback callback);

  void GetUnverifiedPublishers(ledger::UnverifiedPublishersCallback callback);

  void DeleteRecord(uint64_t id, ledger::LegacyResultCallback callback);

  void DeleteAllRecords(ledger::LegacyResultCallback callback);

 private:
  void OnGetReservedAmount(mojom::DBCommandResponsePtr response,
      ledger::GetPendingContributionsTotalCallback callback);

  void OnGetAllRecords(mojom::DBCommandResponsePtr response,
                       ledger::GetPendingContributionsCallback callback);

  void OnGetUnverifiedPublishers(mojom::DBCommandResponsePtr response,
                                 ledger::UnverifiedPublishersCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PENDING_CONTRIBUTION_H_
