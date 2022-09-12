/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_CONTRIBUTION_INFO_PUBLISHERS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_CONTRIBUTION_INFO_PUBLISHERS_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

class DatabaseContributionInfoPublishers: public DatabaseTable {
 public:
  explicit DatabaseContributionInfoPublishers(LedgerImpl* ledger);
  ~DatabaseContributionInfoPublishers() override;

  void InsertOrUpdate(mojom::DBTransaction* transaction,
                      mojom::ContributionInfoPtr info);

  void GetRecordByContributionList(
      const std::vector<std::string>& contribution_ids,
      ContributionPublisherListCallback callback);

  void GetContributionPublisherPairList(
      const std::vector<std::string>& contribution_ids,
      ContributionPublisherPairListCallback callback);

  void UpdateContributedAmount(const std::string& contribution_id,
                               const std::string& publisher_key,
                               ledger::LegacyResultCallback callback);

 private:
  void OnGetRecordByContributionList(
      mojom::DBCommandResponsePtr response,
      ContributionPublisherListCallback callback);

  void OnGetContributionPublisherInfoMap(
      mojom::DBCommandResponsePtr response,
      ContributionPublisherPairListCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_CONTRIBUTION_INFO_PUBLISHERS_H_
