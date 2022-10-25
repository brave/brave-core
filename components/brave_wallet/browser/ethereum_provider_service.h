/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PROVIDER_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PROVIDER_SERVICE_H_

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
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "services/data_decoder/public/cpp/json_sanitizer.h"
#include "url/origin.h"

class HostContentSettingsMap;
class PrefService;

namespace brave_wallet {

class BraveWalletProviderDelegate;
class BraveWalletService;
class JsonRpcService;
class KeyringService;
class TxService;

class EthereumProviderService final
    : public KeyedService,
      public mojom::EthereumProvider,
      public mojom::JsonRpcServiceObserver,
      public mojom::TxServiceObserver,
      public brave_wallet::mojom::KeyringServiceObserver,
      public content_settings::Observer {
 public:
  using GetAllowedAccountsCallback =
      base::OnceCallback<void(const std::vector<std::string>& accounts,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  using RequestPermissionsError = mojom::RequestPermissionsError;

  EthereumProviderService(const EthereumProviderService&) = delete;
  EthereumProviderService& operator=(const EthereumProviderService&) = delete;
  EthereumProviderService(HostContentSettingsMap* host_content_settings_map,
                          JsonRpcService* json_rpc_service,
                          TxService* tx_service,
                          KeyringService* keyring_service,
                          BraveWalletService* brave_wallet_service,
                          PrefService* prefs);
  ~EthereumProviderService() override;

  mojo::PendingRemote<mojom::EthereumProvider> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::EthereumProvider> receiver,
            std::unique_ptr<BraveWalletProviderDelegate> delegate);

  void SendErrorOnRequest(const mojom::ProviderError& error,
                          const std::string& error_message,
                          RequestCallback callback,
                          base::Value id);
  void Web3ClientVersion(RequestCallback callback, base::Value id);
  void GetAllowedAccounts(mojo::ReceiverId receiver_id,
                          bool include_accounts_when_locked,
                          GetAllowedAccountsCallback callback);
  void AddEthereumChain(const std::string& json_payload,
                        RequestCallback callback,
                        mojo::ReceiverId receiver_id,
                        base::Value id);
  void SwitchEthereumChain(const std::string& chain_id,
                           RequestCallback callback,
                           mojo::ReceiverId receiver_id,
                           base::Value id);

  // Used for eth_sign and personal_sign
  void SignMessage(const std::string& address,
                   const std::string& message,
                   RequestCallback callback,
                   mojo::ReceiverId receiver_id,
                   base::Value id);

  // Used for personal_ecRecover
  void RecoverAddress(const std::string& message,
                      const std::string& signature,
                      RequestCallback callback,
                      base::Value id);

  void GetEncryptionPublicKey(const std::string& address,
                              RequestCallback callback,
                              mojo::ReceiverId receiver_id,
                              base::Value id);
  void Decrypt(const std::string& untrusted_encrypted_data_json,
               const std::string& address,
               const url::Origin& origin,
               RequestCallback callback,
               mojo::ReceiverId receiver_id,
               base::Value id);
  // Used for eth_signTypedData
  // message is for displaying the sign request to users
  // message_to_sign is the hex representation without 0x for eip712 hash
  // domain is the domain separator defined in eip712
  void SignTypedMessage(const std::string& address,
                        const std::string& message,
                        const std::vector<uint8_t>& domain_hash,
                        const std::vector<uint8_t>& primary_hash,
                        base::Value::Dict domain,
                        RequestCallback callback,
                        mojo::ReceiverId receiver_id,
                        base::Value id);
  void ContinueGetAllowedAccounts(
      mojo::ReceiverId receiver_id,
      bool include_accounts_when_locked,
      GetAllowedAccountsCallback callback,
      brave_wallet::mojom::KeyringInfoPtr keyring_info);
  void OnGetAllowedAccounts(bool include_accounts_when_locked,
                            bool keyring_locked,
                            const absl::optional<std::string>& selected_account,
                            GetAllowedAccountsCallback callback,
                            bool success,
                            const std::vector<std::string>& accounts);
  void OnContinueGetAllowedAccounts(RequestCallback callback,
                                    base::Value id,
                                    const std::string& method,
                                    const url::Origin& origin,
                                    const std::vector<std::string>& accounts,
                                    mojom::ProviderError error,
                                    const std::string& error_message);

  // Used for wallet_watchAsset.
  // It will prompt an UI for user to confirm, and add the token into user's
  // visible asset list if user approves.
  // Note that we will use the token data from BlockchainRegistry (for
  // mainnet) or from user asset list if there is an existing token with the
  // same contract address, instead of the token data in the request.
  void AddSuggestToken(mojom::BlockchainTokenPtr token,
                       RequestCallback callback,
                       base::Value id,
                       mojo::ReceiverId receiver_id);

 private:
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest, OnAddEthereumChain);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest,
                           AddAndApproveTransactionError);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest,
                           AddAndApproveTransactionNoPermission);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest,
                           AddAndApprove1559Transaction);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest,
                           AddAndApprove1559TransactionNoChainId);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest,
                           AddAndApprove1559TransactionError);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest,
                           AddAndApprove1559TransactionNoPermission);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest,
                           OnAddEthereumChainRequestCompletedError);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest,
                           OnAddEthereumChainRequestCompletedSuccess);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest,
                           AddAndApproveTransaction);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest,
                           RequestEthereumPermissionsNoPermission);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest,
                           RequestEthereumPermissionsNoWallet);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest,
                           RequestEthereumPermissionsLocked);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest, RequestEthCoinbase);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest,
                           RequestEthereumPermissionsWithAccounts);
  friend class EthereumProviderServiceUnitTest;

  // mojom::BraveWalletProvider:
  void Init(
      mojo::PendingRemote<mojom::EventsListener> events_listener) override;
  void Request(base::Value input, RequestCallback callback) override;
  void Enable(EnableCallback callback) override;
  void Send(const std::string& method,
            base::Value params,
            SendCallback callback) override;
  void GetChainId(GetChainIdCallback callback) override;
  void IsLocked(IsLockedCallback callback) override;
  void SetRequestUrl(const GURL& url) override;

  // mojom::JsonRpcServiceObserver
  void ChainChangedEvent(const std::string& chain_id,
                         mojom::CoinType coin) override;
  void OnAddEthereumChainRequestCompleted(const std::string& chain_id,
                                          const std::string& error) override;
  void OnIsEip1559Changed(const std::string& chain_id,
                          bool is_eip1559) override {}
  void OnSwitchEthereumChainRequested(const std::string& chain_id,
                                      const GURL& origin) {}

  // mojom::TxServiceObserver
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) override {}
  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx_info) override {}
  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) override;

  void OnAddEthereumChain(mojo::ReceiverId receiver_id,
                          const std::string& chain_id,
                          mojom::ProviderError error,
                          const std::string& error_message);

  void OnChainApprovalResult(const std::string& chain_id,
                             const std::string& error);
  void OnAddUnapprovedTransaction(RequestCallback callback,
                                  mojo::ReceiverId receiver_id,
                                  base::Value id,
                                  const std::string& tx_meta_id,
                                  mojom::ProviderError error,
                                  const std::string& error_message);
  void OnAddUnapprovedTransactionAdapter(RequestCallback callback,
                                         mojo::ReceiverId receiver_id,
                                         base::Value id,
                                         bool success,
                                         const std::string& tx_meta_id,
                                         const std::string& error_message);
  void ContinueAddAndApproveTransaction(
      RequestCallback callback,
      mojo::ReceiverId receiver_id,
      base::Value id,
      mojom::TxDataPtr tx_data,
      const std::string& from,
      const url::Origin& origin,
      const std::vector<std::string>& allowed_accounts,
      mojom::ProviderError error,
      const std::string& error_message);

  void ContinueAddAndApprove1559Transaction(RequestCallback callback,
                                            mojo::ReceiverId receiver_id,
                                            base::Value id,
                                            mojom::TxData1559Ptr tx_data,
                                            const std::string& from,
                                            const url::Origin& origin,
                                            const std::string& chain_id);
  void ContinueAddAndApprove1559TransactionWithAccounts(
      RequestCallback callback,
      mojo::ReceiverId receiver_id,
      base::Value id,
      mojom::TxData1559Ptr tx_data,
      const std::string& from,
      const url::Origin& origin,
      const std::vector<std::string>& allowed_accounts,
      mojom::ProviderError error,
      const std::string& error_message);
  void ContinueSignMessage(const std::string& address,
                           const std::string& message,
                           std::vector<uint8_t>&& message_to_sign,
                           const absl::optional<std::string>& domain_hash,
                           const absl::optional<std::string>& primary_hash,
                           bool is_eip712,
                           RequestCallback callback,
                           base::Value id,
                           const url::Origin& origin,
                           const std::vector<std::string>& allowed_accounts,
                           mojom::ProviderError error,
                           const std::string& error_message);
  bool CheckAccountAllowed(const std::string& account,
                           const std::vector<std::string>& allowed_accounts);
  void UpdateKnownAccounts();
  void OnUpdateKnownAccounts(const std::vector<std::string>& allowed_accounts,
                             mojom::ProviderError error,
                             const std::string& error_message);

  void ContinueGetDefaultKeyringInfo(RequestCallback callback,
                                     mojo::ReceiverId receiver_id,
                                     base::Value id,
                                     const std::string& normalized_json_request,
                                     const url::Origin& origin,
                                     mojom::NetworkInfoPtr chain);
  void ContinueGetEncryptionPublicKey(
      RequestCallback callback,
      mojo::ReceiverId receiver_id,
      base::Value id,
      const std::string& address,
      const url::Origin& origin,
      const std::vector<std::string>& allowed_accounts,
      mojom::ProviderError error,
      const std::string& error_message);
  void ContinueDecryptWithSanitizedJson(RequestCallback callback,
                                        mojo::ReceiverId receiver_id,
                                        base::Value id,
                                        const std::string& address,
                                        const url::Origin& origin,
                                        data_decoder::JsonSanitizer::Result);
  void ContinueDecryptWithAllowedAccounts(
      RequestCallback callback,
      mojo::ReceiverId receiver_id,
      base::Value id,
      const std::string& version,
      const std::vector<uint8_t>& nonce,
      const std::vector<uint8_t>& ephemeral_public_key,
      const std::vector<uint8_t>& ciphertext,
      const std::string& address,
      const url::Origin& origin,
      const std::vector<std::string>& allowed_accounts,
      mojom::ProviderError error,
      const std::string& error_message);

  void OnGetNetworkAndDefaultKeyringInfo(
      RequestCallback callback,
      mojo::ReceiverId receiver_id,
      base::Value id,
      const std::string& normalized_json_request,
      const url::Origin& origin,
      mojom::NetworkInfoPtr chain,
      mojom::KeyringInfoPtr keyring_info);

  // content_settings::Observer:
  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type) override;

  void OnSignMessageRequestProcessed(RequestCallback callback,
                                     base::Value id,
                                     const std::string& address,
                                     std::vector<uint8_t>&& message,
                                     bool is_eip712,
                                     bool approved,
                                     mojom::ByteArrayStringUnionPtr signature,
                                     const absl::optional<std::string>& error);

  // KeyringServiceObserver
  void KeyringCreated(const std::string& keyring_id) override {}
  void KeyringRestored(const std::string& keyring_id) override {}
  void KeyringReset() override {}
  void Locked() override;
  void Unlocked() override;
  void BackedUp() override {}
  void AccountsChanged() override {}
  void AutoLockMinutesChanged() override {}
  void SelectedAccountChanged(mojom::CoinType coin) override;

  void CommonRequestOrSendAsync(base::ValueView input_value,
                                mojo::ReceiverId receiver_id,
                                RequestCallback callback);

  void RequestEthereumPermissions(RequestCallback callback,
                                  mojo::ReceiverId receiver_id,
                                  base::Value id,
                                  const std::string& method,
                                  const url::Origin& origin);
  void ContinueRequestEthereumPermissionsKeyringInfo(
      RequestCallback callback,
      mojo::ReceiverId receiver_id,
      base::Value id,
      const std::string& method,
      const url::Origin& origin,
      brave_wallet::mojom::KeyringInfoPtr keyring_info);
  void ContinueRequestEthereumPermissions(
      RequestCallback callback,
      mojo::ReceiverId receiver_id,
      base::Value id,
      const std::string& method,
      const url::Origin& origin,
      const std::vector<std::string>& requested_accounts,
      bool success,
      const std::vector<std::string>& allowed_accounts);
  void OnRequestEthereumPermissions(
      RequestCallback callback,
      base::Value id,
      const std::string& method,
      const url::Origin& origin,
      RequestPermissionsError error,
      const absl::optional<std::vector<std::string>>& allowed_accounts);

  void OnReceiverDisconnected();

  raw_ptr<HostContentSettingsMap> host_content_settings_map_ = nullptr;
  mojo::ReceiverSet<mojom::EthereumProvider> receivers_;
  // Map of delegates keyed by ReceiverId. Delegate will be deleited when the
  // receiver disconnected.
  // When calling EthereumProviderService::Bind, delegate must be passed along.
  // Note that receivers_.current_receiver() would only be valid when receiving
  // incoming mojo method call so we need to store ReceiverId for async callback
  // if we need to access the delegate later.
  base::flat_map<mojo::ReceiverId, std::unique_ptr<BraveWalletProviderDelegate>>
      delegates_;
  mojo::RemoteSet<mojom::EventsListener> events_listeners_;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  raw_ptr<TxService> tx_service_ = nullptr;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  raw_ptr<BraveWalletService> brave_wallet_service_ = nullptr;
  base::flat_map<std::string, RequestCallback> chain_callbacks_;
  base::flat_map<std::string, base::Value> chain_ids_;
  base::flat_map<std::string, mojo::ReceiverId> chain_receiver_ids_;
  base::flat_map<std::string, RequestCallback> add_tx_callbacks_;
  base::flat_map<std::string, base::Value> add_tx_ids_;
  RequestCallback pending_request_ethereum_permissions_callback_;
  mojo::ReceiverId pending_request_ethereum_permissions_receiver_id_;
  base::Value pending_request_ethereum_permissions_id_;
  url::Origin pending_request_ethereum_permissions_origin_;
  std::string pending_request_ethereum_permissions_method_;
  mojo::Receiver<mojom::JsonRpcServiceObserver> rpc_observer_receiver_{this};
  mojo::Receiver<mojom::TxServiceObserver> tx_observer_receiver_{this};
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_observer_receiver_{this};
  std::vector<std::string> known_allowed_accounts;
  bool first_known_accounts_check = true;
  PrefService* prefs_ = nullptr;
  bool wallet_onboarding_shown_ = false;
  base::WeakPtrFactory<EthereumProviderService> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PROVIDER_SERVICE_H_
