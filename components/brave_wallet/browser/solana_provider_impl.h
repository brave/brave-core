/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_IMPL_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/content_settings/core/browser/content_settings_observer.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {

class BraveWalletProviderDelegate;
class BraveWalletService;
class JsonRpcService;
class KeyringService;
class SolanaMessage;
class SolanaTransaction;
class TxService;

class SolanaProviderImpl final : public mojom::SolanaProvider,
                                 public KeyringServiceObserverBase,
                                 public mojom::TxServiceObserver,
                                 public content_settings::Observer {
 public:
  using RequestPermissionsError = mojom::RequestPermissionsError;

  SolanaProviderImpl(HostContentSettingsMap& host_content_settings_map,
                     BraveWalletService* brave_wallet_service,
                     std::unique_ptr<BraveWalletProviderDelegate> delegate);
  ~SolanaProviderImpl() override;
  SolanaProviderImpl(const SolanaProviderImpl&) = delete;
  SolanaProviderImpl& operator=(const SolanaProviderImpl&) = delete;

  void Init(mojo::PendingRemote<mojom::SolanaEventsListener> events_listener)
      override;
  void Connect(std::optional<base::Value::Dict> arg,
               ConnectCallback callback) override;
  void Disconnect() override;
  void IsConnected(IsConnectedCallback callback) override;
  void GetPublicKey(GetPublicKeyCallback callback) override;
  void SignTransaction(mojom::SolanaSignTransactionParamPtr param,
                       SignTransactionCallback callback) override;
  void SignAllTransactions(
      std::vector<mojom::SolanaSignTransactionParamPtr> params,
      SignAllTransactionsCallback callback) override;
  void SignAndSendTransaction(mojom::SolanaSignTransactionParamPtr param,
                              std::optional<base::Value::Dict> send_options,
                              SignAndSendTransactionCallback callback) override;
  void SignMessage(const std::vector<uint8_t>& blob_msg,
                   const std::optional<std::string>& display_encoding,
                   SignMessageCallback callback) override;
  void Request(base::Value::Dict arg, RequestCallback callback) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(SolanaProviderImplUnitTest, GetDeserializedMessage);
  FRIEND_TEST_ALL_PREFIXES(SolanaProviderImplUnitTest,
                           ConnectWithNoSolanaAccount);

  bool IsAccountConnected(const mojom::AccountInfo& account);
  void OnConnect(
      const std::vector<mojom::AccountInfoPtr>& requested_accounts,
      ConnectCallback callback,
      RequestPermissionsError error,
      const std::optional<std::vector<std::string>>& allowed_accounts);

  void OnSignMessageRequestProcessed(
      const std::vector<uint8_t>& blob_msg,
      const mojom::AccountInfoPtr& account,
      SignMessageCallback callback,
      bool approved,
      mojom::EthereumSignatureBytesPtr hw_signature,
      const std::optional<std::string>& error);
  void ContinueSignTransaction(
      std::optional<std::pair<SolanaMessage, std::vector<uint8_t>>> msg_pair,
      mojom::SolanaSignTransactionParamPtr param,
      const mojom::AccountInfoPtr& account,
      const std::string& chain_id,
      SignTransactionCallback callback,
      bool is_valid,
      mojom::SolanaProviderError error,
      const std::string& error_message);
  void OnSignTransactionRequestProcessed(
      std::unique_ptr<SolanaTransaction> tx,
      const mojom::AccountInfoPtr& account,
      SignTransactionCallback callback,
      bool approved,
      std::vector<mojom::SolanaSignaturePtr> hw_signatures,
      const std::optional<std::string>& error);
  void ContinueSignAllTransactions(
      std::vector<mojom::SolanaTxDataPtr> tx_datas,
      std::vector<std::unique_ptr<SolanaTransaction>> txs,
      std::vector<std::vector<uint8_t>> raw_messages,
      mojom::AccountInfoPtr account,
      const std::string& chain_id,
      SignAllTransactionsCallback callback,
      const std::vector<bool>& is_valids);
  void OnSignAllTransactionsRequestProcessed(
      const std::vector<std::unique_ptr<SolanaTransaction>>& txs,
      mojom::AccountInfoPtr account,
      SignAllTransactionsCallback callback,
      bool approved,
      std::vector<mojom::SolanaSignaturePtr> signatures,
      const std::optional<std::string>& error);
  void OnAddUnapprovedTransaction(SignAndSendTransactionCallback callback,
                                  bool success,
                                  const std::string& tx_meta_id,
                                  const std::string& error_message);

  // Returns a pair of SolanaMessage and a raw message byte array.
  std::optional<std::pair<SolanaMessage, std::vector<uint8_t>>>
  GetDeserializedMessage(const std::string& encoded_serialized_msg);

  void OnRequestConnect(RequestCallback callback,
                        mojom::SolanaProviderError error,
                        const std::string& error_message,
                        const std::string& public_key);
  void OnRequestSignTransaction(RequestCallback callback,
                                mojom::SolanaProviderError error,
                                const std::string& error_message,
                                const std::vector<uint8_t>& serialized_tx,
                                mojom::SolanaMessageVersion version);
  void OnRequestSignAllTransactions(
      RequestCallback callback,
      mojom::SolanaProviderError error,
      const std::string& error_message,
      const std::vector<std::vector<uint8_t>>& serialized_tx,
      const std::vector<mojom::SolanaMessageVersion>& versions);

  // mojom::KeyringServiceObserverBase:
  void Unlocked() override;
  void SelectedDappAccountChanged(mojom::CoinType coin,
                                  mojom::AccountInfoPtr account) override;

  // mojom::TxServiceObserver
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) override {}
  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx_info) override {}
  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) override;
  void OnTxServiceReset() override {}

  // content_settings::Observer:
  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type) override;

  base::flat_map<std::string, SignAndSendTransactionCallback>
      sign_and_send_tx_callbacks_;
  // Pending callback and arg are for waiting user unlock before connect
  ConnectCallback pending_connect_callback_;
  std::optional<base::Value::Dict> pending_connect_arg_;

  const raw_ref<HostContentSettingsMap> host_content_settings_map_;
  bool account_creation_shown_ = false;
  mojo::Remote<mojom::SolanaEventsListener> events_listener_;
  raw_ptr<BraveWalletService, DanglingUntriaged> brave_wallet_service_ =
      nullptr;
  raw_ptr<KeyringService, DanglingUntriaged> keyring_service_ = nullptr;
  raw_ptr<TxService, DanglingUntriaged> tx_service_ = nullptr;
  raw_ptr<JsonRpcService, DanglingUntriaged> json_rpc_service_ = nullptr;
  mojo::Receiver<mojom::KeyringServiceObserver> keyring_observer_receiver_{
      this};
  mojo::Receiver<mojom::TxServiceObserver> tx_observer_receiver_{this};
  base::ScopedObservation<HostContentSettingsMap, content_settings::Observer>
      host_content_settings_map_observation_{this};
  std::unique_ptr<BraveWalletProviderDelegate> delegate_;
  base::WeakPtrFactory<SolanaProviderImpl> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_IMPL_H_
