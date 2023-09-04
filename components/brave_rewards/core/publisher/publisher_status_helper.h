/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_STATUS_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_STATUS_HELPER_H_

#include <vector>

#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace publisher {

// Refreshes the publisher status for each entry in the specified list
void RefreshPublisherStatus(RewardsEngineImpl& engine,
                            std::vector<mojom::PublisherInfoPtr>&& info_list,
                            GetRecurringTipsCallback callback);

}  // namespace publisher
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_STATUS_HELPER_H_
