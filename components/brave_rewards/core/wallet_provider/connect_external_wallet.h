/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_CONNECT_EXTERNAL_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_CONNECT_EXTERNAL_WALLET_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ref.h"
#include "base/types/expected.h"
#include "brave/components/brave_rewards/core/endpoints/common/post_connect.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace wallet_provider {

class ConnectExternalWallet {
 public:
  explicit ConnectExternalWallet(RewardsEngineImpl& engine);

  virtual ~ConnectExternalWallet();

  std::string GenerateLoginURL();

  void Run(const base::flat_map<std::string, std::string>& query_parameters,
           ConnectExternalWalletCallback);

  struct OAuthInfo {
    std::string one_time_string;
    std::string code_verifier;
    std::string code;
  };

  void SetOAuthStateForTesting(const OAuthInfo& oauth_info) {
    oauth_info_ = oauth_info;
  }

  const OAuthInfo& GetOAuthStateForTesting() const { return oauth_info_; }

 protected:
  virtual const char* WalletType() const = 0;

  virtual std::string GetOAuthLoginURL() const = 0;

  virtual void Authorize(ConnectExternalWalletCallback) = 0;

  void OnConnect(ConnectExternalWalletCallback,
                 std::string&& token,
                 std::string&& address,
                 endpoints::PostConnect::Result&&) const;

  const raw_ref<RewardsEngineImpl> engine_;
  OAuthInfo oauth_info_;

 private:
  base::expected<std::string, mojom::ConnectExternalWalletResult> GetCode(
      const base::flat_map<std::string, std::string>& query_parameters,
      const std::string& current_one_time_string) const;
};

}  // namespace wallet_provider
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_CONNECT_EXTERNAL_WALLET_H_
