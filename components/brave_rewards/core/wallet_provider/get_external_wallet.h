/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_GET_EXTERNAL_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_GET_EXTERNAL_WALLET_H_

#include <string>

#include "brave/components/brave_rewards/core/ledger.h"

namespace brave_rewards::core {
class LedgerImpl;

namespace wallet_provider {

class GetExternalWallet {
 public:
  explicit GetExternalWallet(LedgerImpl*);

  virtual ~GetExternalWallet();

  void Run(GetExternalWalletCallback) const;

 protected:
  virtual const char* WalletType() const = 0;

  virtual void OnTransferTokens(GetExternalWalletCallback,
                                mojom::Result,
                                std::string drain_id) const;

  LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace wallet_provider
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_GET_EXTERNAL_WALLET_H_
