/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_BITFLYER_CONNECT_BITFLYER_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_BITFLYER_CONNECT_BITFLYER_WALLET_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/wallet_provider/connect_external_wallet.h"

namespace ledger {
class LedgerImpl;

namespace endpoint {
class BitflyerServer;
}

namespace bitflyer {

class ConnectBitFlyerWallet : public wallet_provider::ConnectExternalWallet {
 public:
  explicit ConnectBitFlyerWallet(LedgerImpl*);

  ~ConnectBitFlyerWallet() override;

 private:
  const char* WalletType() const override;

  void Authorize(OAuthInfo&&,
                 ledger::ConnectExternalWalletCallback) const override;

  void OnAuthorize(ledger::ConnectExternalWalletCallback,
                   mojom::Result,
                   std::string&& token,
                   std::string&& address,
                   std::string&& linking_info) const;

  std::unique_ptr<endpoint::BitflyerServer> bitflyer_server_;
};

}  // namespace bitflyer
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_BITFLYER_CONNECT_BITFLYER_WALLET_H_
