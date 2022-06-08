/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
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
  void Connect(absl::optional<base::Value> arg,
               ConnectCallback callback) override;
  void Disconnect() override;
  void IsConnected(IsConnectedCallback callback) override;
  void GetPublicKey(GetPublicKeyCallback callback) override;
  void SignTransaction(const std::string& encoded_serialized_msg,
                       SignTransactionCallback callback) override;
  void SignAllTransactions(
      const std::vector<std::string>& encoded_serialized_msgs,
      SignAllTransactionsCallback callback) override;
  void SignAndSendTransaction(const std::string& encoded_serialized_msg,
                              absl::optional<base::Value> send_options,
                              SignAndSendTransactionCallback callback) override;
  void SignMessage(const std::vector<uint8_t>& blob_msg,
                   const absl::optional<std::string>& display_encoding,
                   SignMessageCallback callback) override;
  void Request(base::Value arg, RequestCallback callback) override;

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
                                     const std::string& signature,
                                     const std::string& error);
  void OnSignTransactionRequestProcessed(const SolanaTransaction& tx,
                                         SignTransactionCallback callback,
                                         bool approved);
  void OnSignAllTransactionsRequestProcessed(
      const std::vector<SolanaTransaction>& txs,
      SignAllTransactionsCallback callback,
      bool approved);
  void OnAddUnapprovedTransaction(SignAndSendTransactionCallback callback,
                                  bool success,
                                  const std::string& tx_meta_id,
                                  const std::string& error_message);

  absl::optional<SolanaMessage> GetDeserializedMessage(
      const std::string& encoded_serialized_msg,
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

  // This set is used to maintain connected status for each frame, it is a
  // separate non persistent status from site permission. It depends on if a
  // site successfully call connect or not, calling disconnect will remove
  // itself from this set. Note that site permission is required for a site to
  // do connect, if an user reject the connect request, connect would fail. On
  // the other hand, if the user approve the connect request, site permission
  // will be saved and future connect from the same site will not ask user for
  // permission again until the permission is removed.
  base::flat_set<std::string> connected_set_;
  base::flat_map<std::string, SignAndSendTransactionCallback>
      sign_and_send_tx_callbacks_;
  // Pending callback and arg are for waiting user unlock before connect
  ConnectCallback pending_connect_callback_;
  absl::optional<base::Value> pending_connect_arg_;

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
