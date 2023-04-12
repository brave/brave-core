/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_STATUS_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_STATUS_HELPER_H_

#include <vector>

#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace ledger {
class LedgerImpl;

namespace publisher {

// Refreshes the publisher status for each entry in the specified list
void RefreshPublisherStatus(LedgerImpl* ledger,
                            std::vector<mojom::PublisherInfoPtr>&& info_list,
                            ledger::GetRecurringTipsCallback callback);

// Refreshes the publisher status for each entry in the specified list
void RefreshPublisherStatus(
    LedgerImpl* ledger,
    std::vector<mojom::PendingContributionInfoPtr>&& list,
    ledger::GetPendingContributionsCallback callback);

}  // namespace publisher
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_STATUS_HELPER_H_
