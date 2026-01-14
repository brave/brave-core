/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/secp256k1_hd_keyring.h"

#include <algorithm>
#include <utility>

#include "base/containers/map_util.h"

namespace brave_wallet {

Secp256k1HDKeyring::Secp256k1HDKeyring() = default;

Secp256k1HDKeyring::~Secp256k1HDKeyring() = default;

std::vector<std::string> Secp256k1HDKeyring::GetHDAccountsForTesting() const {
  std::vector<std::string> addresses;
  for (auto& acc : accounts_) {
    addresses.push_back(GetAddressInternal(*acc));
  }
  return addresses;
}

bool Secp256k1HDKeyring::RemoveImportedAccount(const std::string& address) {
  return imported_accounts_.erase(address) != 0;
}

std::optional<std::string> Secp256k1HDKeyring::AddNewHDAccount(uint32_t index) {
  if (!accounts_root_) {
    return std::nullopt;
  }

  if (accounts_.size() != index) {
    return std::nullopt;
  }

  auto new_account = DeriveAccount(index);
  if (!new_account) {
    return std::nullopt;
  }

  auto address = GetAddressInternal(*new_account);
  accounts_.push_back(std::move(new_account));

  return address;
}

bool Secp256k1HDKeyring::RemoveHDAccount(uint32_t index) {
  if (accounts_.size() - 1 != index) {
    return false;
  }
  accounts_.pop_back();
  return true;
}

std::optional<std::string> Secp256k1HDKeyring::ImportAccount(
    base::span<const uint8_t> private_key) {
  auto private_key_fixed_size =
      private_key.to_fixed_extent<kSecp256k1PrivateKeySize>();
  if (!private_key_fixed_size) {
    return std::nullopt;
  }
  std::unique_ptr<HDKey> hd_key =
      HDKey::GenerateFromPrivateKey(*private_key_fixed_size);
  if (!hd_key) {
    return std::nullopt;
  }

  std::string address = GetAddressInternal(*hd_key);

  if (imported_accounts_.contains(address)) {
    return std::nullopt;
  }

  if (std::ranges::any_of(accounts_, [&](auto& acc) {
        return GetAddressInternal(*acc) == address;
      })) {
    return std::nullopt;
  }

  imported_accounts_[address] = std::move(hd_key);
  return address;
}

HDKey* Secp256k1HDKeyring::GetHDKeyFromAddress(const std::string& address) {
  auto* imported_account = base::FindPtrOrNull(imported_accounts_, address);
  if (imported_account) {
    return imported_account;
  }
  for (auto& acc : accounts_) {
    if (GetAddressInternal(*acc) == address) {
      return acc.get();
    }
  }
  return nullptr;
}

}  // namespace brave_wallet
