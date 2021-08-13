/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_IMPL_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller_events_observer.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

class BraveWalletProviderDelegate;
class EthJsonRpcController;

class BraveWalletProviderImpl final
    : public mojom::BraveWalletProvider,
      public mojom::EthJsonRpcControllerObserver {
 public:
  BraveWalletProviderImpl(const BraveWalletProviderImpl&) = delete;
  BraveWalletProviderImpl& operator=(const BraveWalletProviderImpl&) = delete;
  BraveWalletProviderImpl(
      mojo::PendingRemote<mojom::EthJsonRpcController> rpc_controller,
      std::unique_ptr<BraveWalletProviderDelegate> delegate);
  ~BraveWalletProviderImpl() override;

  void Request(const std::string& json_payload,
               bool auto_retry_on_network_change,
               RequestCallback callback) override;
  void RequestEthereumPermissions(
      RequestEthereumPermissionsCallback callback) override;
  void OnRequestEthereumPermissions(RequestEthereumPermissionsCallback callback,
                                    const std::vector<std::string>& accounts);
  void GetChainId(GetChainIdCallback callback) override;
  void GetAllowedAccounts(GetAllowedAccountsCallback callback) override;
  void OnGetAllowedAccounts(GetAllowedAccountsCallback callback,
                            bool success,
                            const std::vector<std::string>& accounts);
  void Init(
      mojo::PendingRemote<mojom::EventsListener> events_listener) override;

  void ChainChangedEvent(const std::string& chain_id) override;

 private:
  void OnConnectionError();

  std::unique_ptr<BraveWalletProviderDelegate> delegate_;
  mojo::Remote<mojom::EventsListener> events_listener_;
  mojo::Remote<mojom::EthJsonRpcController> rpc_controller_;
  mojo::Receiver<mojom::EthJsonRpcControllerObserver> observer_receiver_{this};
  base::WeakPtrFactory<BraveWalletProviderImpl> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_IMPL_H_
