/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_WALLET_PROPERTIES_HANDLER_
#define BAT_LEDGER_WALLET_PROPERTIES_HANDLER_


#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

using WalletProperties = ledger::mojom::WalletProperties;
using WalletPropertiesPtr = ledger::mojom::WalletPropertiesPtr;

using ExternalWallet = ledger::mojom::ExternalWallet;
using ExternalWalletPtr = ledger::mojom::ExternalWalletPtr;

LEDGER_EXPORT enum WALLET_STATUS {
  NOT_CONNECTED = 0,
  CONNECTED = 1,
  EXPIRED = 2
};


}  // namespace ledger

#endif  // BAT_LEDGER_WALLET_PROPERTIES_HANDLER_
