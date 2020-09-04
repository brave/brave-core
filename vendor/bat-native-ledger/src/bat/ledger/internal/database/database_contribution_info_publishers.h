/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_INFO_PUBLISHERS_H_
#define BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_INFO_PUBLISHERS_H_

#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

class DatabaseContributionInfoPublishers: public DatabaseTable {
 public:
  explicit DatabaseContributionInfoPublishers(LedgerImpl* ledger);
  ~DatabaseContributionInfoPublishers() override;

  void InsertOrUpdate(
      type::DBTransaction* transaction,
      type::ContributionInfoPtr info);

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
  void OnGetRecordByContributionList(
      type::DBCommandResponsePtr response,
      ContributionPublisherListCallback callback);

  void OnGetContributionPublisherInfoMap(
      type::DBCommandResponsePtr response,
      ContributionPublisherPairListCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_CONTRIBUTION_INFO_PUBLISHERS_H_
