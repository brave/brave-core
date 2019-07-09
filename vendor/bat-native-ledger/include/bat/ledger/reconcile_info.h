/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_RECONCILE_INFO_H_
#define BAT_LEDGER_RECONCILE_INFO_H_

#include <map>
#include <string>

#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

using ContributionRetry = mojom::ContributionRetry;

using ReconcileInfo = mojom::ReconcileInfo;
using ReconcileInfoPtr = mojom::ReconcileInfoPtr;

}  // namespace ledger

#endif  // BAT_LEDGER_RECONCILE_INFO_H_
