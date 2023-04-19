/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_UPHOLD_CONNECT_UPHOLD_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_UPHOLD_CONNECT_UPHOLD_WALLET_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/timer/timer.h"
#include "brave/components/brave_rewards/core/endpoints/uphold/post_oauth/post_oauth_uphold.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/uphold/uphold_capabilities.h"
#include "brave/components/brave_rewards/core/uphold/uphold_user.h"
#include "brave/components/brave_rewards/core/wallet_provider/connect_external_wallet.h"

namespace ledger::uphold {

class ConnectUpholdWallet : public wallet_provider::ConnectExternalWallet {
 public:
  explicit ConnectUpholdWallet(LedgerImpl& ledger);

  ~ConnectUpholdWallet() override;

 private:
  const char* WalletType() const override;

  void Authorize(OAuthInfo&&,
                 ledger::ConnectExternalWalletCallback) const override;

  void OnAuthorize(ledger::ConnectExternalWalletCallback,
                   endpoints::PostOAuthUphold::Result&&) const;

  void OnGetUser(ledger::ConnectExternalWalletCallback,
                 const std::string& access_token,
                 mojom::Result,
                 const User&) const;

  void OnGetCapabilities(ledger::ConnectExternalWalletCallback,
                         const std::string& access_token,
                         mojom::Result,
                         ledger::uphold::Capabilities) const;

  void OnCreateCard(ledger::ConnectExternalWalletCallback,
                    const std::string& access_token,
                    mojom::Result,
                    std::string&& id) const;

  void CheckEligibility();

  void OnGetUser(mojom::Result, const User&) const;

  void OnGetCapabilities(mojom::Result, Capabilities) const;

  base::RepeatingTimer eligibility_checker_;
};

}  // namespace ledger::uphold

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_UPHOLD_CONNECT_UPHOLD_WALLET_H_
