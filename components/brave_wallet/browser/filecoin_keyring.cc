/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/filecoin_keyring.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

FilecoinKeyring::FilecoinKeyring() = default;
FilecoinKeyring::~FilecoinKeyring() = default;

FilecoinKeyring::Type FilecoinKeyring::type() const {
  return kDefault;
}

std::string FilecoinKeyring::ImportAccount(const std::vector<uint8_t>& private_key) {
  std::unique_ptr<HDKey> hd_key = HDKey::GenerateFromPrivateKey(private_key);
  if (!hd_key)
    return std::string();

  const std::string address = GetAddressInternal(hd_key.get());
  // Account already exists
  if (imported_accounts_[address])
    return std::string();
  // Check if it is duplicate in derived accounts
  for (size_t i = 0; i < accounts_.size(); ++i) {
    if (GetAddress(i) == address)
      return std::string();
  }

  imported_accounts_[address] = std::move(hd_key);
  return address;
}

std::string FilecoinKeyring::GetAddress(size_t index) const {
  if (accounts_.empty() || index >= accounts_.size())
    return std::string();
  return GetAddressInternal(accounts_[index].get());
}

void FilecoinKeyring::SignTransaction(const std::string& address,
                                EthTransaction* tx,
                                uint256_t chain_id) {
  HDKey* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key || !tx)
    return;

  const std::vector<uint8_t> message = tx->GetMessageToSign(chain_id);
  int recid;
  const std::vector<uint8_t> signature = hd_key->Sign(message, &recid);
  tx->ProcessSignature(signature, recid, chain_id);
}

std::vector<uint8_t> FilecoinKeyring::SignMessage(const std::string& address,
                                            const std::vector<uint8_t>& message,
                                            uint256_t chain_id,
                                            bool is_eip712) {
  HDKey* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key)
    return std::vector<uint8_t>();

  std::vector<uint8_t> hash;
  if (!is_eip712) {
    std::string prefix("\x19");
    prefix += std::string("Ethereum Signed Message:\n" +
                          base::NumberToString(message.size()));
    std::vector<uint8_t> hash_input(prefix.begin(), prefix.end());
    hash_input.insert(hash_input.end(), message.begin(), message.end());
    hash = KeccakHash(hash_input);
  } else {
    // eip712 hash is Keccak
    if (message.size() != 32)
      return std::vector<uint8_t>();

    hash = message;
  }

  int recid;
  std::vector<uint8_t> signature = hd_key->Sign(hash, &recid);
  uint8_t v = chain_id ? recid + chain_id * 2 + 35 : recid + 27;
  signature.push_back(v);

  return signature;
}

void FilecoinKeyring::ConstructRootHDKey(const std::vector<uint8_t>& seed,
                                         const std::string& hd_path) {
  if (!seed.empty()) {
    master_key_ = HDKey::GenerateFromSeed(seed);
    if (master_key_) {
      root_ = master_key_->DeriveChildFromPath(hd_path);
    }
  }
}

HDKey* FilecoinKeyring::GetHDKeyFromAddress(const std::string& address) {
  const auto imported_accounts_iter = imported_accounts_.find(address);
  if (imported_accounts_iter != imported_accounts_.end())
    return imported_accounts_iter->second.get();
  for (size_t i = 0; i < accounts_.size(); ++i) {
    if (GetAddress(i) == address)
      return accounts_[i].get();
  }
  return nullptr;
}

}  // namespace brave_wallet
