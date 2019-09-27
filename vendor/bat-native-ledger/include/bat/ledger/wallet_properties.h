/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_WALLET_PROPERTIES_HANDLER_
#define BAT_LEDGER_WALLET_PROPERTIES_HANDLER_

#include <map>
#include <string>

#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

using WalletProperties = ledger::mojom::WalletProperties;
using WalletPropertiesPtr = ledger::mojom::WalletPropertiesPtr;

using ExternalWallet = ledger::mojom::ExternalWallet;
using ExternalWalletPtr = ledger::mojom::ExternalWalletPtr;

using TransferFee = ledger::mojom::TransferFee;
using TransferFeePtr = ledger::mojom::TransferFeePtr;
using TransferFeeList = std::map<std::string, TransferFeePtr>;

const char kWalletAnonymous[] = "anonymous";
const char kWalletUphold[] = "uphold";

LEDGER_EXPORT enum WalletStatus {
  NOT_CONNECTED = 0,
  CONNECTED = 1,
  VERIFIED = 2,
  DISCONNECTED_NOT_VERIFIED = 3,
  DISCONNECTED_VERIFIED = 4,
  PENDING = 5
};

}  // namespace ledger

#endif  // BAT_LEDGER_WALLET_PROPERTIES_HANDLER_
