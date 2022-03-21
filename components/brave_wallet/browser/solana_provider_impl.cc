/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_provider_impl.h"

#include <utility>
#include <vector>

#include "base/notreached.h"
#include "base/values.h"

namespace brave_wallet {

SolanaProviderImpl::SolanaProviderImpl() = default;
SolanaProviderImpl::~SolanaProviderImpl() = default;

void SolanaProviderImpl::Init(
    ::mojo::PendingRemote<mojom::SolanaEventsListener> events_listener) {
  if (!events_listener_.is_bound()) {
    events_listener_.Bind(std::move(events_listener));
  }
}

void SolanaProviderImpl::Connect(absl::optional<base::Value> arg,
                                 ConnectCallback callback) {
  // TODO(darkdh): handle onlyIfTrusted when it exists
  NOTIMPLEMENTED();
  std::move(callback).Run(mojom::SolanaProviderError::kInternalError, "", "");
}

void SolanaProviderImpl::Disconnect() {
  NOTIMPLEMENTED();
}

void SolanaProviderImpl::IsConnected(IsConnectedCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

void SolanaProviderImpl::GetPublicKey(GetPublicKeyCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run("");
}

void SolanaProviderImpl::SignTransaction(
    const std::string& encoded_serialized_msg,
    SignTransactionCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(mojom::SolanaProviderError::kInternalError, "",
                          std::vector<uint8_t>());
}

void SolanaProviderImpl::SignAllTransactions(
    const std::vector<std::string>& encoded_serialized_msgs,
    SignAllTransactionsCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(mojom::SolanaProviderError::kInternalError, "",
                          std::vector<std::vector<uint8_t>>());
}

void SolanaProviderImpl::SignAndSendTransaction(
    const std::string& encoded_serialized_msg,
    SignAndSendTransactionCallback callback) {
  base::Value result(base::Value::Type::DICTIONARY);
  result.SetStringKey("publicKey",
                      "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");
  result.SetStringKey("signature",
                      "As4N6cok5f7nhXp56Hdw8dWZpUnY8zjYKzBqK45CexE1qNPCqt6Y"
                      "2gnZduGgqASDD1c6QULBRypVa9BikoxWpGA");
  std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
                          std::move(result));
  // NOTIMPLEMENTED();
  // std::move(callback).Run(mojom::SolanaProviderError::kInternalError, "",
  // std::move(result));
}

void SolanaProviderImpl::SignMessage(
    const std::string& encoded_msg,
    const absl::optional<std::string>& display_encoding,
    SignMessageCallback callback) {
  base::Value result(base::Value::Type::DICTIONARY);
  result.SetStringKey("publicKey",
                      "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");
  result.SetStringKey("signature",
                      "As4N6cok5f7nhXp56Hdw8dWZpUnY8zjYKzBqK45CexE1qNPCqt6Y"
                      "2gnZduGgqASDD1c6QULBRypVa9BikoxWpGA");
  std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
                          std::move(result));
  // NOTIMPLEMENTED();
  // std::move(callback).Run(mojom::SolanaProviderError::kInternalError, "",
  // std::move(result));
}

void SolanaProviderImpl::Request(base::Value arg, RequestCallback callback) {
  base::Value result(base::Value::Type::DICTIONARY);
  result.SetStringKey("publicKey",
                      "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");
  result.SetStringKey("signature",
                      "As4N6cok5f7nhXp56Hdw8dWZpUnY8zjYKzBqK45CexE1qNPCqt6Y"
                      "2gnZduGgqASDD1c6QULBRypVa9BikoxWpGA");
  std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
                          std::move(result), absl::nullopt);
  // std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
  //                        std::move(result), "connect");
}

}  // namespace brave_wallet
