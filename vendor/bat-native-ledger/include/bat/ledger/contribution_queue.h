/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_CONTRIBUTION_QUEUE_H_
#define BAT_LEDGER_CONTRIBUTION_QUEUE_H_

#include <string>
#include <vector>

#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

using ContributionQueue = ledger::mojom::ContributionQueue;
using ContributionQueuePtr = ledger::mojom::ContributionQueuePtr;
using ContributionQueueList = std::vector<ledger::mojom::ContributionQueuePtr>;

using ContributionQueuePublisher = ledger::mojom::ContributionQueuePublisher;
using ContributionQueuePublisherPtr =
    ledger::mojom::ContributionQueuePublisherPtr;
using ContributionQueuePublisherList =
    std::vector<ContributionQueuePublisherPtr>;

}  // namespace ledger

#endif  // BAT_LEDGER_CONTRIBUTION_QUEUE_H_
