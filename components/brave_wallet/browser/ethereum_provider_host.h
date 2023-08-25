/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PROVIDER_HOST_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PROVIDER_HOST_H_

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

class EthereumProviderHost final : public mojom::EthereumProvider {
 public:
  EthereumProviderHost(const EthereumProviderHost&) = delete;
  EthereumProviderHost& operator=(const EthereumProviderHost&) = delete;
  EthereumProviderHost();
  ~EthereumProviderHost() override;

  // For binding EthereumProviderService
  mojo::PendingReceiver<mojom::EthereumProvider> BindRemote();

 private:
  friend class EthereumProviderServiceUnitTest;
  // mojom::BraveWalletProvider:
  void Init(
      mojo::PendingRemote<mojom::EventsListener> events_listener) override;
  void Request(base::Value input, RequestCallback callback) override;
  void Enable(EnableCallback callback) override;
  void Send(const std::string& method,
            base::Value params,
            SendCallback callback) override;
  void SendAsync(base::Value input, SendAsyncCallback callback) override;
  void GetChainId(GetChainIdCallback callback) override;
  void IsLocked(IsLockedCallback callback) override;

  mojo::Remote<mojom::EthereumProvider> ethereum_provider_service_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PROVIDER_HOST_H_
