/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_BITFLYER_GET_BITFLYER_WALLET_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_BITFLYER_GET_BITFLYER_WALLET_H_

#include <string>

#include "bat/ledger/internal/wallet_provider/get_external_wallet.h"

namespace ledger {
class LedgerImpl;

namespace bitflyer {

class GetBitFlyerWallet : public wallet_provider::GetExternalWallet {
 public:
  explicit GetBitFlyerWallet(LedgerImpl*);

  ~GetBitFlyerWallet() override;

 private:
  const char* WalletType() const override;
};

}  // namespace bitflyer
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_BITFLYER_GET_BITFLYER_WALLET_H_
