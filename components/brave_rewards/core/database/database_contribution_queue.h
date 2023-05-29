/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/database/database_contribution_queue_publishers.h"
#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal::database {

using GetFirstContributionQueueCallback =
    std::function<void(mojom::ContributionQueuePtr)>;

class DatabaseContributionQueue {
 public:
  void InsertOrUpdate(mojom::ContributionQueuePtr info,
                      LegacyResultCallback callback);

  void GetFirstRecord(GetFirstContributionQueueCallback callback);

  void MarkRecordAsComplete(const std::string& id,
                            LegacyResultCallback callback);

 private:
  void OnInsertOrUpdate(
      mojom::DBCommandResponsePtr response,
      std::shared_ptr<mojom::ContributionQueuePtr> shared_queue,
      LegacyResultCallback callback);

  void OnGetFirstRecord(mojom::DBCommandResponsePtr response,
                        GetFirstContributionQueueCallback callback);

  void OnGetPublishers(
      std::vector<mojom::ContributionQueuePublisherPtr> list,
      std::shared_ptr<mojom::ContributionQueuePtr> shared_queue,
      GetFirstContributionQueueCallback callback);

  DatabaseContributionQueuePublishers publishers_;
};

}  // namespace brave_rewards::internal::database

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_
