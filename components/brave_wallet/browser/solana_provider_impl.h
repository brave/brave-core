/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_wallet {
class KeyringService;
class SolanaProviderDelegate;

class SolanaProviderImpl final : public mojom::SolanaProvider {
 public:
  SolanaProviderImpl(KeyringService* keyring_service,
                     std::unique_ptr<SolanaProviderDelegate> delegate);
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
                              SignAndSendTransactionCallback callback) override;
  void SignMessage(const std::string& encoded_msg,
                   const absl::optional<std::string>& display_encoding,
                   SignMessageCallback callback) override;
  void Request(base::Value arg, RequestCallback callback) override;

 private:
  void OnConnect(ConnectCallback callback,
                 const std::string& account,
                 mojom::SolanaProviderError error,
                 const std::string& error_message);

  mojo::Remote<mojom::SolanaEventsListener> events_listener_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  std::unique_ptr<SolanaProviderDelegate> delegate_;
  base::WeakPtrFactory<SolanaProviderImpl> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_PROVIDER_IMPL_H_
