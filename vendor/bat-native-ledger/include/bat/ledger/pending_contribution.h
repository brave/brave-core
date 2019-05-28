/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_PENDING_CONTRIBUTION_HANDLER_
#define BAT_LEDGER_PENDING_CONTRIBUTION_HANDLER_

#include <vector>

#include "bat/ledger/export.h"
#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

using PendingContribution = mojom::PendingContribution;
using PendingContributionPtr = mojom::PendingContributionPtr;
using PendingContributionList = std::vector<PendingContributionPtr>;

using PendingContributionInfo = mojom::PendingContributionInfo;
using PendingContributionInfoPtr = mojom::PendingContributionInfoPtr;
using PendingContributionInfoList = std::vector<PendingContributionInfoPtr>;

}  // namespace ledger

#endif  // BAT_LEDGER_PENDING_CONTRIBUTION_HANDLER_
