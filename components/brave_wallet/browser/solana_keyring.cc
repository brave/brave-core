/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_keyring.h"

#include <memory>
#include <utility>

#include "brave/components/brave_wallet/browser/internal/hd_key_ed25519.h"

namespace brave_wallet {

HDKeyring::Type SolanaKeyring::type() const {
  return HDKeyring::kSolana;
}

void SolanaKeyring::ConstructRootHDKey(const std::vector<uint8_t>& seed,
                                       const std::string& hd_path) {
  if (!seed.empty()) {
    std::unique_ptr<HDKeyEd25519> hd_key = HDKeyEd25519::GenerateFromSeed(seed);
    master_key_ = std::unique_ptr<HDKeyBase>{hd_key.release()};
    if (master_key_) {
      root_ = master_key_->DeriveChildFromPath(hd_path);
    }
  }
}

void SolanaKeyring::AddAccounts(size_t number) {
  size_t cur_accounts_number = accounts_.size();
  for (size_t i = cur_accounts_number; i < cur_accounts_number + number; ++i) {
    if (root_) {
      accounts_.push_back(root_->DeriveChild(i)->DeriveChild(0));
    }
  }
}

std::vector<uint8_t> SolanaKeyring::SignMessage(
    const std::string& address,
    const std::vector<uint8_t>& message) {
  HDKeyBase* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key)
    return std::vector<uint8_t>();

  return hd_key->Sign(message);
}

std::string SolanaKeyring::ImportAccount(
    const std::vector<uint8_t>& private_key) {
  std::unique_ptr<HDKeyEd25519> hd_key =
      HDKeyEd25519::GenerateFromPrivateKey(private_key);
  if (!hd_key)
    return std::string();

  const std::string address = GetAddressInternal(hd_key.get());
  if (!AddImportedAddress(address, std::move(hd_key))) {
    return std::string();
  }

  return address;
}

std::string SolanaKeyring::GetAddressInternal(HDKeyBase* hd_key_base) const {
  if (!hd_key_base)
    return std::string();
  HDKeyEd25519* hd_key = static_cast<HDKeyEd25519*>(hd_key_base);
  return hd_key->GetBase58EncodedPublicKey();
}

}  // namespace brave_wallet
