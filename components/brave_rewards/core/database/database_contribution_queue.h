/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/database/database_contribution_queue_publishers.h"
#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal {
namespace database {

using GetFirstContributionQueueCallback =
    base::OnceCallback<void(mojom::ContributionQueuePtr)>;

class DatabaseContributionQueue : public DatabaseTable {
 public:
  explicit DatabaseContributionQueue(RewardsEngine& engine);
  ~DatabaseContributionQueue() override;

  void InsertOrUpdate(mojom::ContributionQueuePtr info,
                      ResultCallback callback);

  void GetFirstRecord(GetFirstContributionQueueCallback callback);

  void MarkRecordAsComplete(const std::string& id, ResultCallback callback);

 private:
  void OnInsertOrUpdate(ResultCallback callback,
                        mojom::ContributionQueuePtr queue,
                        mojom::DBCommandResponsePtr response);

  void OnGetFirstRecord(GetFirstContributionQueueCallback callback,
                        mojom::DBCommandResponsePtr response);

  void OnGetPublishers(mojom::ContributionQueuePtr queue,
                       GetFirstContributionQueueCallback callback,
                       std::vector<mojom::ContributionQueuePublisherPtr> list);

  DatabaseContributionQueuePublishers publishers_;
  base::WeakPtrFactory<DatabaseContributionQueue> weak_factory_{this};
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_CONTRIBUTION_QUEUE_H_
