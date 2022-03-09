/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_IMPL_H_

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class SolanaProviderImpl final : public mojom::SolanaProvider {
 public:
  SolanaProviderImpl();
  ~SolanaProviderImpl() override;
  SolanaProviderImpl(const SolanaProviderImpl&) = delete;
  SolanaProviderImpl& operator=(const SolanaProviderImpl&) = delete;

  void Connect(absl::optional<base::Value> arg,
               ConnectCallback callback) override;
  void Disconnect() override;
  void IsConnected(IsConnectedCallback callback) override;

 private:
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_IMPL_H_
