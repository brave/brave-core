/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/hd_keyring.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"

namespace brave_wallet {

HDKeyring::HDKeyring() = default;
HDKeyring::~HDKeyring() = default;

HDKeyring::Type HDKeyring::type() const {
  return kDefault;
}

void HDKeyring::ConstructRootHDKey(const std::vector<uint8_t>& seed,
                                   const std::string& hd_path) {
  if (!seed.empty()) {
    master_key_ = HDKey::GenerateFromSeed(seed);
    if (master_key_) {
      root_ = master_key_->DeriveChildFromPath(hd_path);
    }
  }
}

void HDKeyring::AddAccounts(size_t number) {
  size_t cur_accounts_number = accounts_.size();
  for (size_t i = cur_accounts_number; i < cur_accounts_number + number; ++i) {
    if (root_) {
      accounts_.push_back(root_->DeriveChild(i));
    }
  }
}

std::vector<std::string> HDKeyring::GetAccounts() const {
  std::vector<std::string> addresses;
  for (size_t i = 0; i < accounts_.size(); ++i) {
    addresses.push_back(GetAddress(i));
  }
  return addresses;
}

absl::optional<size_t> HDKeyring::GetAccountIndex(
    const std::string& address) const {
  for (size_t i = 0; i < accounts_.size(); ++i) {
    if (GetAddress(i) == address) {
      return i;
    }
  }
  return absl::nullopt;
}

size_t HDKeyring::GetAccountsNumber() const {
  return accounts_.size();
}

void HDKeyring::RemoveAccount() {
  accounts_.pop_back();
}

std::string HDKeyring::ImportAccount(const std::vector<uint8_t>& private_key) {
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

size_t HDKeyring::GetImportedAccountsNumber() const {
  return imported_accounts_.size();
}

bool HDKeyring::RemoveImportedAccount(const std::string& address) {
  return imported_accounts_.erase(address) != 0;
}

std::string HDKeyring::GetAddress(size_t index) const {
  if (accounts_.empty() || index >= accounts_.size())
    return std::string();
  return GetAddressInternal(accounts_[index].get());
}

std::string HDKeyring::GetAddressInternal(const HDKey* hd_key) const {
  if (!hd_key)
    return std::string();
  const std::vector<uint8_t> public_key = hd_key->GetUncompressedPublicKey();
  // trim the header byte 0x04
  const std::vector<uint8_t> pubkey_no_header(public_key.begin() + 1,
                                              public_key.end());
  EthAddress addr = EthAddress::FromPublicKey(pubkey_no_header);

  // TODO(darkdh): chain id op code
  return addr.ToChecksumAddress();
}

void HDKeyring::SignTransaction(const std::string& address,
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

std::vector<uint8_t> HDKeyring::SignMessage(
    const std::string& address,
    const std::vector<uint8_t>& message) {
  HDKey* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key)
    return std::vector<uint8_t>();

  std::string prefix("\x19");
  prefix += std::string("Ethereum Signed Message:\n" +
                        base::NumberToString(message.size()));
  std::vector<uint8_t> hash_input(prefix.begin(), prefix.end());
  hash_input.insert(hash_input.end(), message.begin(), message.end());
  const std::vector<uint8_t> hash = KeccakHash(hash_input);

  return hd_key->Sign(hash);
}

HDKey* HDKeyring::GetHDKeyFromAddress(const std::string& address) {
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
