/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_WALLET_PROPERTIES_HANDLER_
#define BAT_LEDGER_WALLET_PROPERTIES_HANDLER_

#include <string>

namespace ledger {

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
