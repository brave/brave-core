/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CONTRIBUTION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CONTRIBUTION_INFO_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/database/database_contribution_info_publishers.h"
#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal {
namespace database {

using GetContributionInfoCallback =
    base::OnceCallback<void(mojom::ContributionInfoPtr)>;

class DatabaseContributionInfo : public DatabaseTable {
 public:
  explicit DatabaseContributionInfo(RewardsEngine& engine);
  ~DatabaseContributionInfo() override;

  void InsertOrUpdate(mojom::ContributionInfoPtr info, ResultCallback callback);

  void GetRecord(const std::string& contribution_id,
                 GetContributionInfoCallback callback);

  void GetAllRecords(ContributionInfoListCallback callback);

  void GetOneTimeTips(const mojom::ActivityMonth month,
                      const int year,
                      GetOneTimeTipsCallback callback);

  void GetNotCompletedRecords(ContributionInfoListCallback callback);

  void UpdateStep(const std::string& contribution_id,
                  mojom::ContributionStep step,
                  ResultCallback callback);

  void UpdateStepAndCount(const std::string& contribution_id,
                          mojom::ContributionStep step,
                          int32_t retry_count,
                          ResultCallback callback);

  void UpdateContributedAmount(const std::string& contribution_id,
                               const std::string& publisher_key,
                               ResultCallback callback);

  void FinishAllInProgressRecords(ResultCallback callback);

 private:
  void OnGetRecord(GetContributionInfoCallback callback,
                   mojom::DBCommandResponsePtr response);

  void OnGetPublishers(mojom::ContributionInfoPtr contribution,
                       GetContributionInfoCallback callback,
                       std::vector<mojom::ContributionPublisherPtr> list);

  void OnGetOneTimeTips(GetOneTimeTipsCallback callback,
                        mojom::DBCommandResponsePtr response);

  void OnGetList(ContributionInfoListCallback callback,
                 mojom::DBCommandResponsePtr response);

  void OnGetListPublishers(
      std::vector<mojom::ContributionInfoPtr> contributions,
      ContributionInfoListCallback callback,
      std::vector<mojom::ContributionPublisherPtr> list);

  DatabaseContributionInfoPublishers publishers_;
  base::WeakPtrFactory<DatabaseContributionInfo> weak_factory_{this};
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CONTRIBUTION_INFO_H_
