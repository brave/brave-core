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
#include "base/memory/raw_ptr.h"
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
class JsonRpcService;
class KeyringService;

class BraveWalletProviderImpl final
    : public mojom::BraveWalletProvider,
      public mojom::JsonRpcServiceObserver,
      public mojom::EthTxServiceObserver,
      public brave_wallet::mojom::KeyringServiceObserver,
      public content_settings::Observer {
 public:
  BraveWalletProviderImpl(const BraveWalletProviderImpl&) = delete;
  BraveWalletProviderImpl& operator=(const BraveWalletProviderImpl&) = delete;
  BraveWalletProviderImpl(HostContentSettingsMap* host_content_settings_map,
                          JsonRpcService* json_rpc_service,
                          mojo::PendingRemote<mojom::EthTxService> tx_service,
                          KeyringService* keyring_service,
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
                                    const std::vector<std::string>& accounts,
                                    mojom::ProviderError error,
                                    const std::string& error_message);
  void GetChainId(GetChainIdCallback callback) override;
  void GetAllowedAccounts(bool include_accounts_when_locked,
                          GetAllowedAccountsCallback callback) override;
  void AddEthereumChain(const std::string& json_payload,
                        AddEthereumChainCallback callback) override;
  void SwitchEthereumChain(const std::string& chain_id,
                           SwitchEthereumChainCallback callback) override;
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
  void RecoverAddress(const std::string& address,
                      const std::string& message,
                      RecoverAddressCallback callback) override;
  void SignTypedMessage(const std::string& address,
                        const std::string& message,
                        const std::string& message_to_sign,
                        base::Value domain,
                        SignTypedMessageCallback callback) override;
  void OnGetAllowedAccounts(GetAllowedAccountsCallback callback,
                            const std::vector<std::string>& accounts,
                            mojom::ProviderError error,
                            const std::string& error_message);
  void Init(
      mojo::PendingRemote<mojom::EventsListener> events_listener) override;

  void GetNetworkAndDefaultKeyringInfo(
      GetNetworkAndDefaultKeyringInfoCallback callback) override;
  void IsLocked(IsLockedCallback callback) override;

  void AddSuggestToken(mojom::BlockchainTokenPtr token,
                       AddSuggestTokenCallback callback) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveWalletProviderImplUnitTest, OnAddEthereumChain);
  FRIEND_TEST_ALL_PREFIXES(BraveWalletProviderImplUnitTest,
                           OnAddEthereumChainRequestCompletedError);
  FRIEND_TEST_ALL_PREFIXES(BraveWalletProviderImplUnitTest,
                           OnAddEthereumChainRequestCompletedSuccess);
  friend class BraveWalletProviderImplUnitTest;

  // mojom::JsonRpcServiceObserver
  void ChainChangedEvent(const std::string& chain_id) override;
  void OnAddEthereumChainRequestCompleted(const std::string& chain_id,
                                          const std::string& error) override;
  void OnIsEip1559Changed(const std::string& chain_id,
                          bool is_eip1559) override {}
  void OnSwitchEthereumChainRequested(const std::string& chain_id,
                                      const GURL& origin) {}
  void OnSwitchEthereumChainRequestProcessed(bool approved,
                                             const std::string& chain_id,
                                             const GURL& origin);

  // mojom::EthTxServiceObserver
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) override {}
  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx_info) override {}
  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) override;

  void OnAddEthereumChain(const std::string& chain_id,
                          mojom::ProviderError error,
                          const std::string& error_message);

  void OnChainApprovalResult(const std::string& chain_id,
                             const std::string& error);
  void OnConnectionError();
  void OnAddUnapprovedTransaction(AddAndApproveTransactionCallback callback,
                                  const std::string& tx_meta_id,
                                  mojom::ProviderError error,
                                  const std::string& error_message);
  void OnAddUnapprovedTransactionAdapter(
      AddAndApproveTransactionCallback callback,
      bool success,
      const std::string& tx_meta_id,
      const std::string& error_message);
  void ContinueAddAndApproveTransaction(
      AddAndApproveTransactionCallback callback,
      mojom::TxDataPtr tx_data,
      const std::string& from,
      const std::vector<std::string>& allowed_accounts,
      mojom::ProviderError error,
      const std::string& error_message);

  void ContinueAddAndApprove1559Transaction(
      AddAndApproveTransactionCallback callback,
      mojom::TxData1559Ptr tx_data,
      const std::string& from,
      const std::string& chain_id);
  void ContinueAddAndApprove1559TransactionWithAccounts(
      AddAndApproveTransactionCallback callback,
      mojom::TxData1559Ptr tx_data,
      const std::string& from,
      const std::vector<std::string>& allowed_accounts,
      mojom::ProviderError error,
      const std::string& error_message);
  void ContinueSignMessage(const std::string& address,
                           const std::string& message,
                           std::vector<uint8_t>&& message_to_sign,
                           SignMessageCallback callback,
                           bool is_eip712,
                           const std::vector<std::string>& allowed_accounts,
                           mojom::ProviderError error,
                           const std::string& error_message);
  bool CheckAccountAllowed(const std::string& account,
                           const std::vector<std::string>& allowed_accounts);
  void UpdateKnownAccounts();
  void OnUpdateKnownAccounts(const std::vector<std::string>& allowed_accounts,
                             mojom::ProviderError error,
                             const std::string& error_message);

  void ContinueGetDefaultKeyringInfo(
      GetNetworkAndDefaultKeyringInfoCallback callback,
      mojom::EthereumChainPtr chain);
  void OnGetNetworkAndDefaultKeyringInfo(
      GetNetworkAndDefaultKeyringInfoCallback callback,
      mojom::EthereumChainPtr chain,
      mojom::KeyringInfoPtr keyring_info);

  // content_settings::Observer:
  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type) override;

  void OnSignMessageRequestProcessed(SignMessageCallback callback,
                                     const std::string& address,
                                     std::vector<uint8_t>&& message,
                                     bool is_eip712,
                                     bool approved,
                                     const std::string& signature,
                                     const std::string& error);
  void OnHardwareSignMessageRequestProcessed(SignMessageCallback callback,
                                             const std::string& address,
                                             std::vector<uint8_t>&& message,
                                             bool is_eip712,
                                             bool approved,
                                             const std::string& signature,
                                             const std::string& error);

  // KeyringServiceObserver
  void KeyringCreated() override {}
  void KeyringRestored() override {}
  void KeyringReset() override {}
  void Locked() override;
  void Unlocked() override;
  void BackedUp() override {}
  void AccountsChanged() override {}
  void AutoLockMinutesChanged() override {}
  void SelectedAccountChanged() override;

  int sign_message_id_ = 0;
  raw_ptr<HostContentSettingsMap> host_content_settings_map_ = nullptr;
  std::unique_ptr<BraveWalletProviderDelegate> delegate_;
  mojo::Remote<mojom::EventsListener> events_listener_;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  mojo::Remote<mojom::EthTxService> tx_service_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  raw_ptr<BraveWalletService> brave_wallet_service_ = nullptr;
  base::flat_map<std::string, AddEthereumChainCallback> chain_callbacks_;
  base::flat_map<std::string, AddAndApproveTransactionCallback>
      add_tx_callbacks_;
  RequestEthereumPermissionsCallback
      pending_request_ethereum_permissions_callback_;
  mojo::Receiver<mojom::JsonRpcServiceObserver> rpc_observer_receiver_{this};
  mojo::Receiver<mojom::EthTxServiceObserver> tx_observer_receiver_{this};
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_observer_receiver_{this};
  std::vector<std::string> known_allowed_accounts;
  bool first_known_accounts_check = true;
  PrefService* prefs_ = nullptr;
  base::WeakPtrFactory<BraveWalletProviderImpl> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_PROVIDER_IMPL_H_
