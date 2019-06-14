/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_BALANCE_HANDLER_
#define BAT_LEDGER_BALANCE_HANDLER_


#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

using Balance = ledger::mojom::Balance;
using BalancePtr = ledger::mojom::BalancePtr;

const char kWalletAnonymous[] = "anonymous";
const char kWalletUphold[] = "uphold";

}  // namespace ledger

#endif  // BAT_LEDGER_BALANCE_HANDLER_
