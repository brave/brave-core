/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_TABLE_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_core.mojom.h"
#include "brave/components/brave_rewards/common/mojom/rewards_database.mojom.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngine;

namespace database {

using ContributionPublisherInfoPair =
    std::pair<std::string, mojom::PublisherInfoPtr>;

using ServerPublisherLinksCallback =
    base::OnceCallback<void(const std::map<std::string, std::string>& links)>;

using ServerPublisherAmountsCallback =
    base::OnceCallback<void(const std::vector<double>& amounts)>;

using ContributionQueuePublishersListCallback =
    base::OnceCallback<void(std::vector<mojom::ContributionQueuePublisherPtr>)>;

using ContributionPublisherListCallback =
    base::OnceCallback<void(std::vector<mojom::ContributionPublisherPtr>)>;

using ContributionPublisherPairListCallback =
    base::OnceCallback<void(std::vector<ContributionPublisherInfoPair>)>;

class DatabaseTable {
 public:
  explicit DatabaseTable(RewardsEngine& engine);
  virtual ~DatabaseTable();

 protected:
  const raw_ref<RewardsEngine> engine_;
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_TABLE_H_
