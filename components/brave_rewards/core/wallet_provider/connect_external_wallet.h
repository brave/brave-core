/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_CONNECT_EXTERNAL_WALLET_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_CONNECT_EXTERNAL_WALLET_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/types/expected.h"
#include "brave/components/brave_rewards/core/endpoints/post_connect/post_connect.h"
#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace brave_rewards::internal::wallet_provider {

class ConnectExternalWallet {
 public:
  virtual ~ConnectExternalWallet();

  void Run(const base::flat_map<std::string, std::string>& query_parameters,
           ConnectExternalWalletCallback);

 protected:
  virtual const char* WalletType() const = 0;

  struct OAuthInfo {
    std::string one_time_string, code_verifier, code;
  };

  virtual void Authorize(OAuthInfo&&, ConnectExternalWalletCallback) = 0;

  void OnConnect(ConnectExternalWalletCallback,
                 std::string&& token,
                 std::string&& address,
                 endpoints::PostConnect::Result&&) const;

 private:
  absl::optional<OAuthInfo> ExchangeOAuthInfo(mojom::ExternalWalletPtr) const;

  base::expected<std::string, mojom::ConnectExternalWalletError> GetCode(
      const base::flat_map<std::string, std::string>& query_parameters,
      const std::string& current_one_time_string) const;
};

}  // namespace brave_rewards::internal::wallet_provider

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_WALLET_PROVIDER_CONNECT_EXTERNAL_WALLET_H_
