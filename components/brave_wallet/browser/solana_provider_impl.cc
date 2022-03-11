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

void SolanaProviderImpl::Connect(absl::optional<base::Value> arg,
                                 ConnectCallback callback) {
  // TODO(darkdh): handle onlyIfTrusted when it exists
  // NOTIMPLEMENTED();
  // std::move(callback).Run(mojom::SolanaProviderError::kInternalError, "",
  // "");
  std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "",
                          "BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");
}

void SolanaProviderImpl::Disconnect() {
  // NOTIMPLEMENTED();
}

void SolanaProviderImpl::IsConnected(IsConnectedCallback callback) {
  // NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

void SolanaProviderImpl::GetPublicKey(GetPublicKeyCallback callback) {
  std::move(callback).Run("BrG44HdsEhzapvs8bEqzvkq4egwevS3fRE6ze2ENo6S8");
  // std::move(callback).Run("");
}

void SolanaProviderImpl::SignTransaction(
    const std::string& encoded_serialized_msg,
    SignTransactionCallback callback) {
  std::vector<uint8_t> result = {
      1,   231, 78,  150, 120, 219, 234, 88,  44,  144, 225, 53,  221, 88,  82,
      59,  51,  62,  211, 225, 231, 182, 123, 231, 229, 201, 48,  30,  137, 119,
      233, 102, 88,  31,  65,  88,  147, 197, 72,  166, 241, 126, 26,  59,  239,
      64,  196, 116, 28,  17,  124, 0,   123, 13,  28,  65,  242, 241, 226, 46,
      227, 55,  234, 251, 10,  1,   0,   1,   2,   161, 51,  89,  91,  115, 210,
      217, 212, 76,  159, 171, 200, 40,  150, 157, 70,  197, 71,  24,  44,  209,
      108, 143, 4,   58,  251, 215, 62,  201, 172, 159, 197, 0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   62,  84,
      174, 253, 228, 77,  50,  164, 215, 178, 46,  88,  242, 49,  114, 246, 244,
      48,  9,   18,  36,  41,  160, 254, 174, 6,   207, 115, 11,  58,  220, 167,
      1,   1,   2,   0,   0,   12,  2,   0,   0,   0,   100, 0,   0,   0,   0,
      0,   0,   0};
  std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "", result);
  // NOTIMPLEMENTED();
  // std::move(callback).Run(mojom::SolanaProviderError::kInternalError, "",
  // std::vector<uint8_t>());
}

void SolanaProviderImpl::SignAllTransactions(
    const std::vector<std::string>& encoded_serialized_msgs,
    SignAllTransactionsCallback callback) {
  std::vector<uint8_t> result = {
      1,   231, 78,  150, 120, 219, 234, 88,  44,  144, 225, 53,  221, 88,  82,
      59,  51,  62,  211, 225, 231, 182, 123, 231, 229, 201, 48,  30,  137, 119,
      233, 102, 88,  31,  65,  88,  147, 197, 72,  166, 241, 126, 26,  59,  239,
      64,  196, 116, 28,  17,  124, 0,   123, 13,  28,  65,  242, 241, 226, 46,
      227, 55,  234, 251, 10,  1,   0,   1,   2,   161, 51,  89,  91,  115, 210,
      217, 212, 76,  159, 171, 200, 40,  150, 157, 70,  197, 71,  24,  44,  209,
      108, 143, 4,   58,  251, 215, 62,  201, 172, 159, 197, 0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   62,  84,
      174, 253, 228, 77,  50,  164, 215, 178, 46,  88,  242, 49,  114, 246, 244,
      48,  9,   18,  36,  41,  160, 254, 174, 6,   207, 115, 11,  58,  220, 167,
      1,   1,   2,   0,   0,   12,  2,   0,   0,   0,   100, 0,   0,   0,   0,
      0,   0,   0};
  std::vector<std::vector<uint8_t>> results = {result, result};
  std::move(callback).Run(mojom::SolanaProviderError::kSuccess, "", results);
  // NOTIMPLEMENTED();
  // std::move(callback).Run(mojom::SolanaProviderError::kInternalError, "",
  // std::vector<std::vector<uint8_t>>());
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

}  // namespace brave_wallet
