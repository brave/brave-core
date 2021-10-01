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

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefService;

namespace brave_wallet {

class BraveWalletProviderDelegate;
class EthJsonRpcController;
class TrezorBridgeController;

class BraveWalletProviderImpl final
    : public mojom::BraveWalletProvider,
      public mojom::EthJsonRpcControllerObserver {
 public:
  BraveWalletProviderImpl(const BraveWalletProviderImpl&) = delete;
  BraveWalletProviderImpl& operator=(const BraveWalletProviderImpl&) = delete;
  BraveWalletProviderImpl(
      mojo::PendingRemote<mojom::EthJsonRpcController> rpc_controller,
      mojo::PendingRemote<mojom::EthTxController> tx_controller,
      mojo::PendingRemote<mojom::TrezorBridgeController> trezor_controller,
      std::unique_ptr<BraveWalletProviderDelegate> delegate,
      PrefService* prefs);
  ~BraveWalletProviderImpl() override;

  void Request(const std::string& json_payload,
               bool auto_retry_on_network_change,
               RequestCallback callback) override;
  void RequestEthereumPermissions(
      RequestEthereumPermissionsCallback callback) override;
  void OnRequestEthereumPermissions(RequestEthereumPermissionsCallback callback,
                                    bool success,
                                    const std::vector<std::string>& accounts);
  void GetChainId(GetChainIdCallback callback) override;
  void GetAllowedAccounts(GetAllowedAccountsCallback callback) override;
  void AddEthereumChain(const std::string& json_payload,
                        AddEthereumChainCallback callback) override;
  void AddUnapprovedTransaction(
      mojom::TxDataPtr tx_data,
      const std::string& from,
      AddUnapprovedTransactionCallback callback) override;
  void AddUnapproved1559Transaction(
      mojom::TxData1559Ptr tx_data,
      const std::string& from,
      AddUnapproved1559TransactionCallback callback) override;
  void OnGetAllowedAccounts(GetAllowedAccountsCallback callback,
                            bool success,
                            const std::vector<std::string>& accounts);
  void Init(
      mojo::PendingRemote<mojom::EventsListener> events_listener) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveWalletProviderImplUnitTest, OnAddEthereumChain);
  FRIEND_TEST_ALL_PREFIXES(BraveWalletProviderImplUnitTest,
                           OnAddEthereumChainRequestCompletedError);
  FRIEND_TEST_ALL_PREFIXES(BraveWalletProviderImplUnitTest,
                           OnAddEthereumChainRequestCompletedSuccess);

  // mojom::EthJsonRpcControllerObserver
  void ChainChangedEvent(const std::string& chain_id) override;
  void OnAddEthereumChainRequestCompleted(const std::string& chain_id,
                                          const std::string& error) override;

  void OnAddEthereumChain(const std::string& chain_id, bool accepted);
  void OnChainApprovalResult(const std::string& chain_id,
                             const std::string& error);
  void OnConnectionError();
  void OnAddUnapprovedTransaction(AddUnapprovedTransactionCallback callback,
                                  bool success,
                                  const std::string& tx_meta_id,
                                  const std::string& error_message);
  void OnAddUnapproved1559Transaction(
      AddUnapproved1559TransactionCallback callback,
      bool success,
      const std::string& tx_meta_id,
      const std::string& error_message);
  void ContinueAddUnapprovedTransaction(
      AddUnapprovedTransactionCallback callback,
      mojom::TxDataPtr tx_data,
      const std::string& from,
      bool success,
      const std::vector<std::string>& allowed_accounts);
  void ContinueAddUnapproved1559Transaction(
      AddUnapproved1559TransactionCallback callback,
      mojom::TxData1559Ptr tx_data,
      const std::string& from,
      bool success,
      const std::vector<std::string>& allowed_accounts);
  bool CheckAccountAllowed(const std::string& account,
                           const std::vector<std::string>& allowed_accounts);

  std::unique_ptr<BraveWalletProviderDelegate> delegate_;
  mojo::Remote<mojom::EventsListener> events_listener_;
  mojo::Remote<mojom::EthJsonRpcController> rpc_controller_;
  mojo::Remote<mojom::EthTxController> tx_controller_;
  mojo::Remote<mojom::TrezorBridgeController> trezor_controller_;

  base::flat_map<std::string, AddEthereumChainCallback> chain_callbacks_;
  mojo::Receiver<mojom::EthJsonRpcControllerObserver> observer_receiver_{this};
  PrefService* prefs_ = nullptr;
  base::WeakPtrFactory<BraveWalletProviderImpl> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_IMPL_H_
