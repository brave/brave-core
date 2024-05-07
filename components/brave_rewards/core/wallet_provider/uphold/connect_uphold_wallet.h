/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_UPHOLD_CONNECT_UPHOLD_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_UPHOLD_CONNECT_UPHOLD_WALLET_H_

#include <string>

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
  explicit ConnectUpholdWallet(RewardsEngine& engine);

  ~ConnectUpholdWallet() override;

  void CheckEligibility();

 private:
  const char* WalletType() const override;

  std::string GetOAuthLoginURL() const override;

  void Authorize(ConnectExternalWalletCallback callback) override;

  void OnAuthorize(ConnectExternalWalletCallback,
                   endpoints::PostOAuthUphold::Result&&);

  void OnGetUser(ConnectExternalWalletCallback,
                 const std::string& access_token,
                 mojom::Result,
                 User) const;

  void OnGetCapabilities(ConnectExternalWalletCallback,
                         const std::string& access_token,
                         const std::string& country_id,
                         mojom::Result,
                         Capabilities) const;

  void OnCreateCard(ConnectExternalWalletCallback,
                    const std::string& access_token,
                    const std::string& country_id,
                    mojom::Result,
                    std::string&& id) const;

  void OnGetUserForEligibilityCheck(mojom::Result, User) const;

  void OnGetCapabilitiesForEligibilityCheck(mojom::Result, Capabilities) const;

  UpholdCard card_;
  endpoint::UpholdServer server_;
};

}  // namespace brave_rewards::internal::uphold

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_UPHOLD_CONNECT_UPHOLD_WALLET_H_
