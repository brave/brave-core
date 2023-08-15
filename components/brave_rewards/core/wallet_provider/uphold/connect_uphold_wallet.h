/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_UPHOLD_CONNECT_UPHOLD_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_UPHOLD_CONNECT_UPHOLD_WALLET_H_

#include <string>

#include "base/timer/timer.h"
#include "brave/components/brave_rewards/core/endpoint/uphold/uphold_server.h"
#include "brave/components/brave_rewards/core/endpoints/uphold/post_oauth_uphold.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/uphold/uphold_capabilities.h"
#include "brave/components/brave_rewards/core/uphold/uphold_card.h"
#include "brave/components/brave_rewards/core/uphold/uphold_user.h"
#include "brave/components/brave_rewards/core/wallet_provider/connect_external_wallet.h"

namespace brave_rewards::internal::uphold {

class ConnectUpholdWallet : public wallet_provider::ConnectExternalWallet {
 public:
  explicit ConnectUpholdWallet(RewardsEngineImpl& engine);

  ~ConnectUpholdWallet() override;

 private:
  const char* WalletType() const override;

  void Authorize(OAuthInfo&&, ConnectExternalWalletCallback) override;

  void OnAuthorize(ConnectExternalWalletCallback,
                   endpoints::PostOAuthUphold::Result&&);

  void OnGetUser(ConnectExternalWalletCallback,
                 const std::string& access_token,
                 mojom::Result,
                 const User&) const;

  void OnGetCapabilities(ConnectExternalWalletCallback,
                         const std::string& access_token,
                         const std::string& country_id,
                         mojom::Result,
                         internal::uphold::Capabilities) const;

  void OnCreateCard(ConnectExternalWalletCallback,
                    const std::string& access_token,
                    const std::string& country_id,
                    mojom::Result,
                    std::string&& id) const;

  void CheckEligibility();

  void OnGetUser(mojom::Result, const User&) const;

  void OnGetCapabilities(mojom::Result, Capabilities) const;

  UpholdCard card_;
  endpoint::UpholdServer server_;
  base::RetainingOneShotTimer eligibility_checker_;
};

}  // namespace brave_rewards::internal::uphold

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_UPHOLD_CONNECT_UPHOLD_WALLET_H_
