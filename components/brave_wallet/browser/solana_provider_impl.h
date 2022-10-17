/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_IMPL_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

class BraveWalletProviderDelegate;
class BraveWalletService;
class KeyringService;
class SolanaMessage;
class SolanaTransaction;
class TxService;

class SolanaProviderImpl final : public mojom::SolanaProvider,
                                 public mojom::KeyringServiceObserver,
                                 public mojom::TxServiceObserver {
 public:
  using RequestPermissionsError = mojom::RequestPermissionsError;

  SolanaProviderImpl(KeyringService* keyring_service,
                     BraveWalletService* brave_wallet_service,
                     TxService* tx_service,
                     std::unique_ptr<BraveWalletProviderDelegate> delegate);
  ~SolanaProviderImpl() override;
  SolanaProviderImpl(const SolanaProviderImpl&) = delete;
  SolanaProviderImpl& operator=(const SolanaProviderImpl&) = delete;

  void Init(mojo::PendingRemote<mojom::SolanaEventsListener> events_listener)
      override;
  void Connect(absl::optional<base::Value::Dict> arg,
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
                              absl::optional<base::Value::Dict> send_options,
                              SignAndSendTransactionCallback callback) override;
  void SignMessage(const std::vector<uint8_t>& blob_msg,
                   const absl::optional<std::string>& display_encoding,
                   SignMessageCallback callback) override;
  void Request(base::Value::Dict arg, RequestCallback callback) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(SolanaProviderImplUnitTest, GetDeserializedMessage);

  bool IsAccountConnected(const std::string& account);
  void ContinueConnect(bool is_eagerly_connect,
                       const std::string& selected_account,
                       ConnectCallback callback,
                       bool is_selected_account_allowed);
  void OnConnect(
      const std::string& requested_account,
      ConnectCallback callback,
      RequestPermissionsError error,
      const absl::optional<std::vector<std::string>>& allowed_accounts);

  void OnSignMessageRequestProcessed(const std::vector<uint8_t>& blob_msg,
                                     const std::string& account,
                                     SignMessageCallback callback,
                                     bool approved,
                                     mojom::ByteArrayStringUnionPtr signature,
                                     const absl::optional<std::string>& error);
  void OnSignTransactionRequestProcessed(
      std::unique_ptr<SolanaTransaction> tx,
      const std::string& account,
      SignTransactionCallback callback,
      bool approved,
      mojom::ByteArrayStringUnionPtr signature,
      const absl::optional<std::string>& error);
  void OnSignAllTransactionsRequestProcessed(
      const std::vector<std::unique_ptr<SolanaTransaction>>& txs,
      const std::string& account,
      SignAllTransactionsCallback callback,
      bool approved,
      absl::optional<std::vector<mojom::ByteArrayStringUnionPtr>> signatures,
      const absl::optional<std::string>& error);
  void OnAddUnapprovedTransaction(SignAndSendTransactionCallback callback,
                                  bool success,
                                  const std::string& tx_meta_id,
                                  const std::string& error_message);

  // Returns a pair of SolanaMessage and a raw message byte array.
  absl::optional<std::pair<SolanaMessage, std::vector<uint8_t>>>
  GetDeserializedMessage(const std::string& encoded_serialized_msg,
                         const std::string& account);

  void OnRequestConnect(RequestCallback callback,
                        mojom::SolanaProviderError error,
                        const std::string& error_message,
                        const std::string& public_key);
  void OnRequestSignTransaction(RequestCallback callback,
                                mojom::SolanaProviderError error,
                                const std::string& error_message,
                                const std::vector<uint8_t>& serialized_tx);
  void OnRequestSignAllTransactions(
      RequestCallback callback,
      mojom::SolanaProviderError error,
      const std::string& error_message,
      const std::vector<std::vector<uint8_t>>& serialized_tx);

  // mojom::KeyringServiceObserver
  void KeyringCreated(const std::string& keyring_id) override {}
  void KeyringRestored(const std::string& keyring_id) override {}
  void KeyringReset() override {}
  void Locked() override {}
  void Unlocked() override;
  void BackedUp() override {}
  void AccountsChanged() override {}
  void AutoLockMinutesChanged() override {}
  void SelectedAccountChanged(mojom::CoinType coin) override;

  // mojom::TxServiceObserver
  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) override {}
  void OnUnapprovedTxUpdated(mojom::TransactionInfoPtr tx_info) override {}
  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) override;

  base::flat_map<std::string, SignAndSendTransactionCallback>
      sign_and_send_tx_callbacks_;
  // Pending callback and arg are for waiting user unlock before connect
  ConnectCallback pending_connect_callback_;
  absl::optional<base::Value::Dict> pending_connect_arg_;

  mojo::Remote<mojom::SolanaEventsListener> events_listener_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  raw_ptr<BraveWalletService> brave_wallet_service_ = nullptr;
  raw_ptr<TxService> tx_service_ = nullptr;
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_observer_receiver_{this};
  mojo::Receiver<mojom::TxServiceObserver> tx_observer_receiver_{this};
  std::unique_ptr<BraveWalletProviderDelegate> delegate_;
  base::WeakPtrFactory<SolanaProviderImpl> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_IMPL_H_
