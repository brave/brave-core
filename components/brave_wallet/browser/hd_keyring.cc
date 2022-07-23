/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/hd_keyring.h"

#include <utility>

namespace brave_wallet {

HDKeyring::HDKeyring() = default;
HDKeyring::~HDKeyring() = default;

void HDKeyring::ConstructRootHDKey(const std::vector<uint8_t>& seed,
                                   const std::string& hd_path) {
  if (!seed.empty()) {
    std::unique_ptr<HDKey> hd_key = HDKey::GenerateFromSeed(seed);
    master_key_ = std::unique_ptr<HDKeyBase>{hd_key.release()};
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

bool HDKeyring::AddImportedAddress(const std::string& address,
                                   std::unique_ptr<HDKeyBase> hd_key) {
  // Account already exists
  if (imported_accounts_.find(address) != imported_accounts_.end())
    return false;
  // Check if it is duplicate in derived accounts
  for (size_t i = 0; i < accounts_.size(); ++i) {
    if (GetAddress(i) == address)
      return false;
  }

  imported_accounts_[address] = std::move(hd_key);
  return true;
}

std::string HDKeyring::ImportAccount(const std::vector<uint8_t>& private_key) {
  std::unique_ptr<HDKey> hd_key = HDKey::GenerateFromPrivateKey(private_key);
  if (!hd_key)
    return std::string();

  const std::string address = GetAddressInternal(hd_key.get());
  if (!AddImportedAddress(address, std::move(hd_key))) {
    return std::string();
  }

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

std::string HDKeyring::GetDiscoveryAddress(size_t index) const {
  if (auto key = root_->DeriveChild(index)) {
    return GetAddressInternal(key.get());
  }
  return std::string();
}

std::string HDKeyring::GetEncodedPrivateKey(const std::string& address) {
  HDKeyBase* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key)
    return std::string();

  return hd_key->GetEncodedPrivateKey();
}

std::vector<uint8_t> HDKeyring::SignMessage(
    const std::string& address,
    const std::vector<uint8_t>& message) {
  HDKeyBase* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key)
    return std::vector<uint8_t>();

  return hd_key->Sign(message, nullptr);
}

HDKeyBase* HDKeyring::GetHDKeyFromAddress(const std::string& address) {
  const auto imported_accounts_iter = imported_accounts_.find(address);
  if (imported_accounts_iter != imported_accounts_.end())
    return imported_accounts_iter->second.get();
  for (size_t i = 0; i < accounts_.size(); ++i) {
    if (GetAddress(i) == address)
      return accounts_[i].get();
  }
  return nullptr;
}

bool HDKeyring::HasAddress(const std::string& address) {
  for (size_t i = 0; i < accounts_.size(); ++i) {
    if (GetAddress(i) == address)
      return true;
  }
  return false;
}

bool HDKeyring::HasImportedAddress(const std::string& address) {
  return imported_accounts_.find(address) != imported_accounts_.end();
}

}  // namespace brave_wallet
