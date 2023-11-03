/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/brave_wallet/filecoin_utility_impl.h"

#include <utility>

#include "brave/components/filecoin/rs/src/lib.rs.h"

namespace brave_wallet {

FilecoinUtilityImpl::FilecoinUtilityImpl() = default;
FilecoinUtilityImpl::~FilecoinUtilityImpl() = default;

void FilecoinUtilityImpl::BLSPrivateKeyToPublicKey(
    const std::vector<uint8_t>& private_key,
    BLSPrivateKeyToPublicKeyCallback callback) {
  if (private_key.size() != 32) {
    std::move(callback).Run(absl::nullopt);
    return;
  }

  auto result = filecoin::bls_private_key_to_public_key(
      rust::Slice<const uint8_t>{private_key.data(), private_key.size()});
  std::vector<uint8_t> public_key(result.begin(), result.end());
  if (std::all_of(public_key.begin(), public_key.end(),
                  [](int i) { return i == 0; })) {
    std::move(callback).Run(absl::nullopt);
    return;
  }

  std::move(callback).Run(public_key);
}

void FilecoinUtilityImpl::TransactionSign(
    bool is_mainnet,
    const std::string& transaction,
    const std::vector<uint8_t>& private_key,
    TransactionSignCallback callback) {
  std::move(callback).Run(std::string(filecoin::transaction_sign(
      is_mainnet, transaction,
      rust::Slice<const uint8_t>{private_key.data(), private_key.size()})));
}

}  // namespace brave_wallet
