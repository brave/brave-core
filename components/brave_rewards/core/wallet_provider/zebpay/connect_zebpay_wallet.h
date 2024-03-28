/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_ZEBPAY_CONNECT_ZEBPAY_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_ZEBPAY_CONNECT_ZEBPAY_WALLET_H_

#include <string>

#include "brave/components/brave_rewards/core/endpoints/zebpay/post_oauth_zebpay.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/wallet_provider/connect_external_wallet.h"

namespace brave_rewards::internal {
class RewardsEngine;

namespace zebpay {

class ConnectZebPayWallet : public wallet_provider::ConnectExternalWallet {
 public:
  explicit ConnectZebPayWallet(RewardsEngine& engine);

  ~ConnectZebPayWallet() override;

 private:
  const char* WalletType() const override;

  std::string GetOAuthLoginURL() const override;

  void Authorize(ConnectExternalWalletCallback callback) override;

  void OnAuthorize(ConnectExternalWalletCallback callback,
                   endpoints::PostOAuthZebPay::Result&& result) const;
};

}  // namespace zebpay
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_ZEBPAY_CONNECT_ZEBPAY_WALLET_H_
