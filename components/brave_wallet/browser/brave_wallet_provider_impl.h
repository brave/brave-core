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
#include "components/content_settings/core/browser/content_settings_observer.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

class HostContentSettingsMap;
class PrefService;

namespace brave_wallet {

class BraveWalletProviderDelegate;
class BraveWalletService;
class EthJsonRpcController;
class KeyringController;

class BraveWalletProviderImpl final
    : public mojom::BraveWalletProvider,
      public mojom::EthJsonRpcControllerObserver,
      public mojom::EthTxControllerObserver,
      public brave_wallet::mojom::KeyringControllerObserver,
      public content_settings::Observer {
 public:
  BraveWalletProviderImpl(const BraveWalletProviderImpl&) = delete;
  BraveWalletProviderImpl& operator=(const BraveWalletProviderImpl&) = delete;
  BraveWalletProviderImpl(
      HostContentSettingsMap* host_content_settings_map,
      mojo::PendingRemote<mojom::EthJsonRpcController> rpc_controller,
      mojo::PendingRemote<mojom::EthTxController> tx_controller,
      KeyringController* keyring_controller,
      BraveWalletService* brave_wallet_service,
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
  void AddAndApproveTransaction(
      mojom::TxDataPtr tx_data,
      const std::string& from,
      AddAndApproveTransactionCallback callback) override;
  void AddAndApprove1559Transaction(
      mojom::TxData1559Ptr tx_data,
      const std::string& from,
      AddAndApprove1559TransactionCallback callback) override;
  void SignMessage(const std::string& address,
                   const std::string& message,
                   SignMessageCallback callback) override;
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
  friend class BraveWalletProviderImplUnitTest;

  // mojom::EthJsonRpcControllerObserver
  void ChainChangedEvent(const std::string& chain_id) override;
  void OnAddEthereumChainRequestCompleted(const std::string& chain_id,
                                          const std::string& error) override;
  void OnIsEip1559Changed(const std::string& chain_id,
                          bool is_eip1559) override {}

  // mojom::EthTxControllerObserver
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) override {}
  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx_info) override {}
  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) override;

  void OnAddEthereumChain(const std::string& chain_id, bool accepted);
  void OnChainApprovalResult(const std::string& chain_id,
                             const std::string& error);
  void OnConnectionError();
  void OnAddUnapprovedTransaction(AddAndApproveTransactionCallback callback,
                                  bool success,
                                  const std::string& tx_meta_id,
                                  const std::string& error_message);
  void ContinueAddAndApproveTransaction(
      AddAndApproveTransactionCallback callback,
      mojom::TxDataPtr tx_data,
      const std::string& from,
      bool success,
      const std::vector<std::string>& allowed_accounts);
  void ContinueAddAndApprove1559Transaction(
      AddAndApproveTransactionCallback callback,
      mojom::TxData1559Ptr tx_data,
      const std::string& from,
      bool success,
      const std::vector<std::string>& allowed_accounts);
  void ContinueSignMessage(const std::string& address,
                           std::vector<uint8_t>&& message,
                           SignMessageCallback callback,
                           bool success,
                           const std::vector<std::string>& allowed_accounts);
  bool CheckAccountAllowed(const std::string& account,
                           const std::vector<std::string>& allowed_accounts);
  void UpdateKnownAccounts();
  void OnUpdateKnownAccounts(bool success,
                             const std::vector<std::string>& allowed_accounts);

  // content_settings::Observer:
  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type) override;

  void OnSignMessageRequestProcessed(SignMessageCallback callback,
                                     const std::string& address,
                                     std::vector<uint8_t>&& message,
                                     bool approved);

  // KeyringControllerObserver
  void KeyringCreated() override {}
  void KeyringRestored() override {}
  void Locked() override {}
  void Unlocked() override {}
  void BackedUp() override {}
  void AccountsChanged() override {}
  void AutoLockMinutesChanged() override {}
  void SelectedAccountChanged() override;

  int sign_message_id_ = 0;
  HostContentSettingsMap* host_content_settings_map_;
  std::unique_ptr<BraveWalletProviderDelegate> delegate_;
  mojo::Remote<mojom::EventsListener> events_listener_;
  mojo::Remote<mojom::EthJsonRpcController> rpc_controller_;
  mojo::Remote<mojom::EthTxController> tx_controller_;
  KeyringController* keyring_controller_;
  BraveWalletService* brave_wallet_service_;
  base::flat_map<std::string, AddEthereumChainCallback> chain_callbacks_;
  base::flat_map<std::string, AddAndApproveTransactionCallback>
      add_tx_callbacks_;
  mojo::Receiver<mojom::EthJsonRpcControllerObserver> rpc_observer_receiver_{
      this};
  mojo::Receiver<mojom::EthTxControllerObserver> tx_observer_receiver_{this};
  mojo::Receiver<brave_wallet::mojom::KeyringControllerObserver>
      keyring_observer_receiver_{this};
  std::vector<std::string> known_allowed_accounts;
  bool first_known_accounts_check = true;
  PrefService* prefs_ = nullptr;
  base::WeakPtrFactory<BraveWalletProviderImpl> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_IMPL_H_
