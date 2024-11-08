/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PROVIDER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PROVIDER_IMPL_H_

#include <map>
#include <memory>
#include <optional>
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
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/origin.h"

class HostContentSettingsMap;
class PrefService;

namespace brave_wallet {

class BraveWalletProviderDelegate;
class BraveWalletService;
class JsonRpcService;
class KeyringService;
class TxService;

class EthereumProviderImpl final : public mojom::EthereumProvider,
                                   public mojom::JsonRpcServiceObserver,
                                   public mojom::TxServiceObserver,
                                   public KeyringServiceObserverBase,
                                   public content_settings::Observer,
                                   public EthBlockTracker::Observer,
                                   public EthLogsTracker::Observer {
 public:
  using RequestPermissionsError = mojom::RequestPermissionsError;

  EthereumProviderImpl(const EthereumProviderImpl&) = delete;
  EthereumProviderImpl& operator=(const EthereumProviderImpl&) = delete;
  EthereumProviderImpl(HostContentSettingsMap* host_content_settings_map,
                       BraveWalletService* brave_wallet_service,
                       std::unique_ptr<BraveWalletProviderDelegate> delegate,
                       PrefService* prefs);
  ~EthereumProviderImpl() override;

  void SendErrorOnRequest(const mojom::ProviderError& error,
                          const std::string& error_message,
                          RequestCallback callback,
                          base::Value id);
  void Web3ClientVersion(RequestCallback callback, base::Value id);
  std::optional<std::vector<std::string>> GetAllowedAccounts(
      bool include_accounts_when_locked);
  void AddEthereumChain(const std::string& json_payload,
                        RequestCallback callback,
                        base::Value id);
  void SwitchEthereumChain(const std::string& chain_id,
                           RequestCallback callback,
                           base::Value id);

  // Used for eth_sign and personal_sign
  void SignMessage(const std::string& address,
                   const std::string& message,
                   RequestCallback callback,
                   base::Value id);

  // Used for personal_ecRecover
  void RecoverAddress(const std::string& message,
                      const std::string& signature,
                      RequestCallback callback,
                      base::Value id);

  void EthSubscribe(const std::string& event_type,
                    std::optional<base::Value::Dict> filter,
                    RequestCallback callback,
                    base::Value id);
  void EthUnsubscribe(const std::string& subscription_id,
                      RequestCallback callback,
                      base::Value id);

  void GetEncryptionPublicKey(const std::string& address,
                              RequestCallback callback,
                              base::Value id);
  void Decrypt(const std::string& untrusted_encrypted_data_json,
               const std::string& address,
               const url::Origin& origin,
               RequestCallback callback,
               base::Value id);
  // Used for eth_signTypedData
  void SignTypedMessage(mojom::EthSignTypedDataPtr eth_sign_typed_data,
                        RequestCallback callback,
                        base::Value id);
  void GetAllowedAccountsInternal(RequestCallback callback,
                                  base::Value id,
                                  const std::string& method,
                                  bool include_accounts_when_locked);

  // Used for wallet_watchAsset.
  // It will prompt an UI for user to confirm, and add the token into user's
  // visible asset list if user approves.
  // Note that we will use the token data from BlockchainRegistry (for
  // mainnet) or from user asset list if there is an existing token with the
  // same contract address, instead of the token data in the request.
  void AddSuggestToken(mojom::BlockchainTokenPtr token,
                       RequestCallback callback,
                       base::Value id);

 private:
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest, OnAddEthereumChain);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest,
                           AddAndApproveTransactionError);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest,
                           AddAndApproveTransactionNoPermission);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest,
                           AddAndApprove1559Transaction);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest,
                           AddAndApprove1559TransactionNoChainId);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest,
                           AddAndApprove1559TransactionError);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest,
                           AddAndApprove1559TransactionNoPermission);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest,
                           OnAddEthereumChainRequestCompletedError);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest,
                           OnAddEthereumChainRequestCompletedSuccess);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest,
                           AddAndApproveTransaction);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest,
                           RequestEthereumPermissionsNoPermission);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest,
                           RequestEthereumPermissionsNoWallet);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest,
                           RequestEthereumPermissionsLocked);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest, RequestEthCoinbase);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest,
                           RequestEthereumPermissionsWithAccounts);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest, EthSubscribe);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest, EthSubscribeLogs);
  FRIEND_TEST_ALL_PREFIXES(EthereumProviderImplUnitTest,
                           EthSubscribeLogsFiltered);
  friend class EthereumProviderImplUnitTest;

  mojom::AccountIdPtr FindAuthenticatedAccountByAddress(
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
            base::Value::List params,
            SendCallback callback) override;
  void SendAsync(base::Value input, SendAsyncCallback callback) override;
  void GetChainId(GetChainIdCallback callback) override;
  void IsLocked(IsLockedCallback callback) override;

  // mojom::JsonRpcServiceObserver
  void ChainChangedEvent(const std::string& chain_id,
                         mojom::CoinType coin,
                         const std::optional<url::Origin>& origin) override;
  void OnAddEthereumChainRequestCompleted(const std::string& chain_id,
                                          const std::string& error) override;
  void OnSwitchEthereumChainRequested(const std::string& chain_id,
                                      const GURL& origin) {}
  void OnSwitchEthereumChainRequestProcessed(bool approved,
                                             const std::string& chain_id,
                                             const GURL& origin);

  // mojom::TxServiceObserver
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) override {}
  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx_info) override {}
  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) override;
  void OnTxServiceReset() override {}

  void OnChainApprovalResult(const std::string& chain_id,
                             const std::string& error);
  void OnAddUnapprovedTransaction(RequestCallback callback,
                                  base::Value id,
                                  const std::string& tx_meta_id,
                                  mojom::ProviderError error,
                                  const std::string& error_message);
  void OnAddUnapprovedTransactionAdapter(RequestCallback callback,
                                         base::Value id,
                                         bool success,
                                         const std::string& tx_meta_id,
                                         const std::string& error_message);
  void SignMessageInternal(const mojom::AccountIdPtr& account_id,
                           mojom::SignDataUnionPtr sign_data,
                           std::vector<uint8_t> message_to_sign,
                           RequestCallback callback,
                           base::Value id);
  bool CheckAccountAllowed(const mojom::AccountIdPtr& account_id,
                           const std::vector<std::string>& allowed_accounts);
  void UpdateKnownAccounts();
  void OnUpdateKnownAccounts(const std::vector<std::string>& allowed_accounts,
                             mojom::ProviderError error,
                             const std::string& error_message);
  void ContinueDecryptWithSanitizedJson(
      RequestCallback callback,
      base::Value id,
      const mojom::AccountIdPtr& account_id,
      const url::Origin& origin,
      base::expected<base::Value, std::string> result);
  void SendOrSignTransactionInternal(RequestCallback callback,
                                     base::Value id,
                                     const std::string& normalized_json_request,
                                     bool sign_only);

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
                                     mojom::EthereumSignatureBytesPtr signature,
                                     const std::optional<std::string>& error);

  // KeyringServiceObserverBase:
  void Locked() override;
  void Unlocked() override;
  void SelectedDappAccountChanged(mojom::CoinType coin,
                                  mojom::AccountInfoPtr account) override;

  void CommonRequestOrSendAsync(base::ValueView input_value,
                                RequestCallback request_callback,
                                bool format_json_rpc_response);

  void RequestEthereumPermissions(RequestCallback callback,
                                  base::Value id,
                                  const std::string& method,
                                  const url::Origin& origin);
  void OnRequestEthereumPermissions(
      RequestCallback callback,
      base::Value id,
      const std::string& method,
      const url::Origin& origin,
      RequestPermissionsError error,
      const std::optional<std::vector<std::string>>& allowed_accounts);
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
  bool UnsubscribeBlockObserver(const std::string& subscription_id);

  // EthLogsTracker::Observer:
  void OnLogsReceived(const std::string& subscription,
                      base::Value rawlogs) override;
  bool UnsubscribeLogObserver(const std::string& subscription_id);

  raw_ptr<HostContentSettingsMap> host_content_settings_map_ = nullptr;
  std::unique_ptr<BraveWalletProviderDelegate> delegate_;
  mojo::Remote<mojom::EventsListener> events_listener_;
  raw_ptr<BraveWalletService, DanglingUntriaged> brave_wallet_service_ =
      nullptr;
  raw_ptr<JsonRpcService, DanglingUntriaged> json_rpc_service_ = nullptr;
  raw_ptr<TxService, DanglingUntriaged> tx_service_ = nullptr;
  raw_ptr<KeyringService, DanglingUntriaged> keyring_service_ = nullptr;
  base::flat_map<std::string, RequestCallback> chain_callbacks_;
  base::flat_map<std::string, base::Value> chain_ids_;
  base::flat_map<std::string, RequestCallback> add_tx_callbacks_;
  base::flat_map<std::string, base::Value> add_tx_ids_;
  RequestCallback pending_request_ethereum_permissions_callback_;
  base::Value pending_request_ethereum_permissions_id_;
  url::Origin pending_request_ethereum_permissions_origin_;
  std::string pending_request_ethereum_permissions_method_;
  mojo::Receiver<mojom::JsonRpcServiceObserver> rpc_observer_receiver_{this};
  mojo::Receiver<mojom::TxServiceObserver> tx_observer_receiver_{this};
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_observer_receiver_{this};
  std::vector<std::string> known_allowed_accounts_;
  std::vector<std::string> eth_subscriptions_;
  std::vector<std::string> eth_log_subscriptions_;
  EthBlockTracker eth_block_tracker_;
  EthLogsTracker eth_logs_tracker_;
  bool first_known_accounts_check_ = true;
  const raw_ptr<PrefService, DanglingUntriaged> prefs_ = nullptr;
  bool wallet_onboarding_shown_ = false;
  base::WeakPtrFactory<EthereumProviderImpl> weak_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETHEREUM_PROVIDER_IMPL_H_
