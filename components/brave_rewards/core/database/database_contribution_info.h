/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CONTRIBUTION_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CONTRIBUTION_INFO_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/database/database_contribution_info_publishers.h"
#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal {
namespace database {

using GetContributionInfoCallback =
    std::function<void(mojom::ContributionInfoPtr)>;

class DatabaseContributionInfo : public DatabaseTable {
 public:
  explicit DatabaseContributionInfo(RewardsEngineImpl& engine);
  ~DatabaseContributionInfo() override;

  void InsertOrUpdate(mojom::ContributionInfoPtr info,
                      LegacyResultCallback callback);

  void GetRecord(const std::string& contribution_id,
                 GetContributionInfoCallback callback);

  void GetAllRecords(ContributionInfoListCallback callback);

  void GetOneTimeTips(const mojom::ActivityMonth month,
                      const int year,
                      GetOneTimeTipsCallback callback);

  void GetContributionReport(const mojom::ActivityMonth month,
                             const int year,
                             GetContributionReportCallback callback);

  void GetNotCompletedRecords(ContributionInfoListCallback callback);

  void UpdateStep(const std::string& contribution_id,
                  mojom::ContributionStep step,
                  LegacyResultCallback callback);

  void UpdateStepAndCount(const std::string& contribution_id,
                          mojom::ContributionStep step,
                          int32_t retry_count,
                          LegacyResultCallback callback);

  void UpdateContributedAmount(const std::string& contribution_id,
                               const std::string& publisher_key,
                               LegacyResultCallback callback);

  void FinishAllInProgressRecords(LegacyResultCallback callback);

 private:
  void OnGetRecord(GetContributionInfoCallback callback,
                   mojom::DBCommandResponsePtr response);

  void OnGetPublishers(
      std::vector<mojom::ContributionPublisherPtr> list,
      std::shared_ptr<mojom::ContributionInfoPtr> shared_contribution,
      GetContributionInfoCallback callback);

  void OnGetOneTimeTips(GetOneTimeTipsCallback callback,
                        mojom::DBCommandResponsePtr response);

  void OnGetContributionReport(GetContributionReportCallback callback,
                               mojom::DBCommandResponsePtr response);

  void OnGetContributionReportPublishers(
      std::vector<ContributionPublisherInfoPair> publisher_pair_list,
      std::shared_ptr<std::vector<mojom::ContributionInfoPtr>>
          shared_contributions,
      GetContributionReportCallback callback);

  void OnGetList(ContributionInfoListCallback callback,
                 mojom::DBCommandResponsePtr response);

  void OnGetListPublishers(
      std::vector<mojom::ContributionPublisherPtr> list,
      std::shared_ptr<std::vector<mojom::ContributionInfoPtr>>
          shared_contributions,
      ContributionInfoListCallback callback);

  DatabaseContributionInfoPublishers publishers_;
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CONTRIBUTION_INFO_H_
