/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PROVIDER_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PROVIDER_SERVICE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/eth_block_tracker.h"
#include "brave/components/brave_wallet/browser/eth_logs_tracker.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
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
struct PendingAddChainRequest;
struct PendingAddTxRequest;
struct PendingPermissionRequest;
struct KnownAccountsInfo;

class EthereumProviderService final : public KeyedService,
                                      public mojom::EthereumProvider,
                                      public mojom::JsonRpcServiceObserver,
                                      public mojom::TxServiceObserver,
                                      public KeyringServiceObserverBase,
                                      public content_settings::Observer,
                                      public EthBlockTracker::Observer,
                                      public EthLogsTracker::Observer {
 public:
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
  mojo::ReceiverId Bind(mojo::PendingReceiver<mojom::EthereumProvider> receiver,
                        std::unique_ptr<BraveWalletProviderDelegate> delegate);

  void SendErrorOnRequest(const mojom::ProviderError& error,
                          const std::string& error_message,
                          RequestCallback callback,
                          base::Value id);
  void Web3ClientVersion(RequestCallback callback, base::Value id);
  absl::optional<std::vector<std::string>> GetAllowedAccounts(
      mojo::ReceiverId receiver_id,
      bool include_accounts_when_locked);
  void AddEthereumChain(mojo::ReceiverId receiver_id,
                        const std::string& json_payload,
                        RequestCallback callback,
                        base::Value id);
  void SwitchEthereumChain(mojo::ReceiverId receiver_id,
                           const std::string& chain_id,
                           RequestCallback callback,
                           base::Value id);

  // Used for eth_sign and personal_sign
  void SignMessage(mojo::ReceiverId receiver_id,
                   const std::string& address,
                   const std::string& message,
                   RequestCallback callback,
                   base::Value id);

  // Used for personal_ecRecover
  void RecoverAddress(const std::string& message,
                      const std::string& signature,
                      RequestCallback callback,
                      base::Value id);

  void EthSubscribe(mojo::ReceiverId receiver_id,
                    const std::string& event_type,
                    absl::optional<base::Value::Dict> filter,
                    RequestCallback callback,
                    base::Value id);
  void EthUnsubscribe(mojo::ReceiverId receiver_id,
                      const std::string& subscription_id,
                      RequestCallback callback,
                      base::Value id);

  void GetEncryptionPublicKey(mojo::ReceiverId receiver_id,
                              const std::string& address,
                              RequestCallback callback,
                              base::Value id);
  void Decrypt(mojo::ReceiverId receiver_id,
               const std::string& untrusted_encrypted_data_json,
               const std::string& address,
               const url::Origin& origin,
               RequestCallback callback,
               base::Value id);
  // Used for eth_signTypedData
  // message is for displaying the sign request to users
  // message_to_sign is the hex representation without 0x for eip712 hash
  // domain is the domain separator defined in eip712
  void SignTypedMessage(mojo::ReceiverId receiver_id,
                        const std::string& address,
                        const std::string& message,
                        const std::vector<uint8_t>& domain_hash,
                        const std::vector<uint8_t>& primary_hash,
                        base::Value::Dict domain,
                        RequestCallback callback,
                        base::Value id);
  void GetAllowedAccountsInternal(mojo::ReceiverId receiver_id,
                                  RequestCallback callback,
                                  base::Value id,
                                  const std::string& method,
                                  bool include_accounts_when_locked);

  // Used for wallet_watchAsset.
  // It will prompt an UI for user to confirm, and add the token into user's
  // visible asset list if user approves.
  // Note that we will use the token data from BlockchainRegistry (for
  // mainnet) or from user asset list if there is an existing token with the
  // same contract address, instead of the token data in the request.
  void AddSuggestToken(mojo::ReceiverId receiver_id,
                       mojom::BlockchainTokenPtr token,
                       RequestCallback callback,
                       base::Value id);

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
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest, EthSubscribe);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest, EthSubscribeLogs);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderServiceUnitTest,
                           EthSubscribeLogsFiltered);
  friend class EthereumProviderServiceUnitTest;

  mojom::AccountIdPtr FindAuthenticatedAccountByAddress(
      mojo::ReceiverId receiver_id,
      const std::string& address,
      base::Value& id,
      mojom::EthereumProvider::RequestCallback& callback);
  mojom::AccountIdPtr FindAccountByAddress(const std::string& address);

  // mojom::EthereumProvider:
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

  // mojom::JsonRpcServiceObserver
  void ChainChangedEvent(const std::string& chain_id,
                         mojom::CoinType coin,
                         const absl::optional<url::Origin>& origin) override;
  void OnAddEthereumChainRequestCompleted(const std::string& chain_id,
                                          const std::string& error) override;
  void OnIsEip1559Changed(const std::string& chain_id,
                          bool is_eip1559) override {}

  // mojom::TxServiceObserver
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) override {}
  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx_info) override {}
  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) override;
  void OnTxServiceReset() override {}

  void OnChainApprovalResult(const std::string& chain_id,
                             const std::string& error);
  void OnAddUnapprovedTransaction(mojo::ReceiverId receiver_id,
                                  RequestCallback callback,
                                  base::Value id,
                                  const std::string& tx_meta_id,
                                  mojom::ProviderError error,
                                  const std::string& error_message);
  void OnAddUnapprovedTransactionAdapter(mojo::ReceiverId receiver_id,
                                         RequestCallback callback,
                                         base::Value id,
                                         bool success,
                                         const std::string& tx_meta_id,
                                         const std::string& error_message);
  void SignMessageInternal(mojo::ReceiverId receiver_id,
                           const mojom::AccountIdPtr& account_id,
                           const std::string& domain,
                           const std::string& message,
                           std::vector<uint8_t> message_to_sign,
                           const absl::optional<std::string>& domain_hash,
                           const absl::optional<std::string>& primary_hash,
                           bool is_eip712,
                           RequestCallback callback,
                           base::Value id);
  bool CheckAccountAllowed(const mojom::AccountIdPtr& account_id,
                           const std::vector<std::string>& allowed_accounts);
  void UpdateKnownAccounts();
  void OnUpdateKnownAccounts(const std::vector<std::string>& allowed_accounts,
                             mojom::ProviderError error,
                             const std::string& error_message);

  void ContinueGetDefaultKeyringInfo(mojo::ReceiverId receiver_id,
                                     RequestCallback callback,
                                     base::Value id,
                                     const std::string& normalized_json_request,
                                     const url::Origin& origin,
                                     bool sign_only,
                                     mojom::NetworkInfoPtr chain);
  void ContinueGetEncryptionPublicKey(
      RequestCallback callback,
      base::Value id,
      const std::string& address,
      const url::Origin& origin,
      const std::vector<std::string>& allowed_accounts,
      mojom::ProviderError error,
      const std::string& error_message);
  void ContinueDecryptWithSanitizedJson(mojo::ReceiverId receiver_id,
                                        RequestCallback callback,
                                        base::Value id,
                                        const mojom::AccountIdPtr& account_id,
                                        const url::Origin& origin,
                                        data_decoder::JsonSanitizer::Result);
  void OnGetNetworkAndDefaultKeyringInfo(
      mojo::ReceiverId receiver_id,
      RequestCallback callback,
      base::Value id,
      const std::string& normalized_json_request,
      const url::Origin& origin,
      mojom::NetworkInfoPtr chain,
      bool sign_only,
      mojom::KeyringInfoPtr keyring_info);

  // content_settings::Observer:
  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type) override;

  void OnSignMessageRequestProcessed(RequestCallback callback,
                                     base::Value id,
                                     const mojom::AccountIdPtr& account_id,
                                     std::vector<uint8_t> message,
                                     bool is_eip712,
                                     bool approved,
                                     mojom::ByteArrayStringUnionPtr signature,
                                     const absl::optional<std::string>& error);

  // KeyringServiceObserverBase:
  void Locked() override;
  void Unlocked() override;
  void SelectedDappAccountChanged(mojom::CoinType coin,
                                  mojom::AccountInfoPtr account) override;

  void CommonRequestOrSendAsync(mojo::ReceiverId receiver_id,
                                base::ValueView input_value,
                                RequestCallback request_callback,
                                bool format_json_rpc_response);

  void RequestEthereumPermissions(mojo::ReceiverId receiver_id,
                                  RequestCallback callback,
                                  base::Value id,
                                  const std::string& method,
                                  const url::Origin& origin);
  void OnRequestEthereumPermissions(
      mojo::ReceiverId receiver_id,
      RequestCallback callback,
      base::Value id,
      const std::string& method,
      const url::Origin& origin,
      RequestPermissionsError error,
      const absl::optional<std::vector<std::string>>& allowed_accounts);
  void OnSendRawTransaction(RequestCallback callback,
                            base::Value id,
                            const std::string& tx_hash,
                            mojom::ProviderError error,
                            const std::string& error_message);
  void OnGetBlockByNumber(base::Value result,
                          mojom::ProviderError,
                          const std::string&);

  void OnResponse(bool format_json_rpc_response,
                  RequestCallback callback,
                  base::Value id,
                  base::Value formed_response,
                  const bool reject,
                  const std::string& first_allowed_account,
                  const bool update_bind_js_properties);

  // EthBlockTracker::Observer:
  void OnLatestBlock(const std::string& chain_id, uint256_t block_num) override;
  void OnNewBlock(const std::string& chain_id, uint256_t block_num) override;
  bool UnsubscribeBlockObserver(mojo::ReceiverId receiver_id,
                                const std::string& subscription_id);

  // EthLogsTracker::Observer:
  void OnLogsReceived(const std::string& subscription_id,
                      base::Value rawlogs) override;
  bool UnsubscribeLogObserver(const std::string& subscription_id);

  void OnReceiverDisconnected();
  void OnEventsListenerDisconnected(mojo::RemoteSetElementId remote_id);

  raw_ptr<HostContentSettingsMap> host_content_settings_map_ = nullptr;
  mojo::ReceiverSet<mojom::EthereumProvider> receivers_;
  // Map of delegates keyed by ReceiverId. Delegate will be deleted when the
  // receiver disconnected.
  // When calling EthereumProviderService::Bind, delegate must be passed along.
  // Note that receivers_.current_receiver() would only be valid when receiving
  // incoming mojo method call so we need to store ReceiverId for async callback
  // if we need to access the delegate later.
  base::flat_map<mojo::ReceiverId, std::unique_ptr<BraveWalletProviderDelegate>>
      delegates_;
  // Used to associate receiver and remote for the same EthereumProviderHost so
  // we can access the correct delegate by remote id.
  base::flat_map<mojo::RemoteSetElementId, mojo::ReceiverId> receiver_ids_;
  mojo::RemoteSet<mojom::EventsListener> events_listeners_;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  raw_ptr<TxService> tx_service_ = nullptr;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  raw_ptr<BraveWalletService> brave_wallet_service_ = nullptr;
  // keyed by chain_id and there can only be one request per chain id across all
  // renderers.
  base::flat_map<std::string, PendingAddChainRequest>
      pending_add_chain_requests_;
  // keyed by tx meta id which is guaranteed to be unique in wallet.
  base::flat_map<std::string, PendingAddTxRequest> pending_add_tx_requests_;
  // A map for storing permission requests for different renderers.
  base::flat_map<mojo::ReceiverId, PendingPermissionRequest>
      pending_permission_requests_;
  mojo::Receiver<mojom::JsonRpcServiceObserver> rpc_observer_receiver_{this};
  mojo::Receiver<mojom::TxServiceObserver> tx_observer_receiver_{this};
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_observer_receiver_{this};
  // These needs to be handled per EthereumProviderHost
  base::flat_map<mojo::ReceiverId, KnownAccountsInfo> known_accounts_infos_;
  std::vector<std::pair<mojo::ReceiverId, std::string>> eth_subscriptions_;
  std::vector<std::pair<mojo::ReceiverId, std::string>> eth_log_subscriptions_;
  EthBlockTracker eth_block_tracker_;
  EthLogsTracker eth_logs_tracker_;
  const raw_ptr<PrefService> prefs_ = nullptr;
  bool wallet_onboarding_shown_ = false;
  base::WeakPtrFactory<EthereumProviderService> weak_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PROVIDER_SERVICE_H_
