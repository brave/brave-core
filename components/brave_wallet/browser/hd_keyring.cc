/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/hd_keyring.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"

namespace {

// Get the 32 byte message hash
std::vector<uint8_t> GetMessageHash(const std::vector<uint8_t>& message) {
  std::string prefix("\x19");
  prefix += std::string("Ethereum Signed Message:\n" +
                        base::NumberToString(message.size()));
  std::vector<uint8_t> hash_input(prefix.begin(), prefix.end());
  hash_input.insert(hash_input.end(), message.begin(), message.end());
  return brave_wallet::KeccakHash(hash_input);
}

}  // namespace

namespace brave_wallet {

HDKeyring::HDKeyring() = default;
HDKeyring::~HDKeyring() = default;

HDKeyring::Type HDKeyring::type() const {
  return kDefault;
}

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
  if (imported_accounts_[address])
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

std::string HDKeyring::GetEncodedPrivateKey(const std::string& address) {
  HDKeyBase* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key)
    return std::string();

  return hd_key->GetEncodedPrivateKey();
}

std::string HDKeyring::GetAddressInternal(HDKeyBase* hd_key_base) const {
  if (!hd_key_base)
    return std::string();
  HDKey* hd_key = static_cast<HDKey*>(hd_key_base);
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
  HDKeyBase* hd_key = GetHDKeyFromAddress(address);
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
  NOTREACHED() << "ETH shouldn't use this signing function";
  return std::vector<uint8_t>();
}

std::vector<uint8_t> HDKeyring::SignMessage(const std::string& address,
                                            const std::vector<uint8_t>& message,
                                            uint256_t chain_id,
                                            bool is_eip712) {
  HDKeyBase* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key)
    return std::vector<uint8_t>();

  std::vector<uint8_t> hash;
  if (!is_eip712) {
    hash = GetMessageHash(message);
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

// static
bool HDKeyring::RecoverAddress(const std::vector<uint8_t>& message,
                               const std::vector<uint8_t>& signature,
                               std::string* address) {
  CHECK(address);
  // A compact ECDSA signature (recovery id byte + 64 bytes).
  if (signature.size() != 65)
    return false;

  std::vector<uint8_t> signature_only = signature;
  uint8_t v = signature_only.back();
  if (v < 27) {
    VLOG(1) << "v should be >= 27";
    return false;
  }

  // v = chain_id ? recid + chain_id * 2 + 35 : recid + 27;
  // So recid = v - 27 when chain_id is 0
  uint8_t recid = v - 27;
  signature_only.pop_back();
  std::vector<uint8_t> hash = GetMessageHash(message);

  // Public keys (in scripts) are given as 04 <x> <y> where x and y are 32
  // byte big-endian integers representing the coordinates of a point on the
  // curve or in compressed form given as <sign> <x> where <sign> is 0x02 if
  // y is even and 0x03 if y is odd.
  HDKey key;
  std::vector<uint8_t> public_key =
      key.Recover(false, hash, signature_only, recid);
  if (public_key.size() != 65) {
    VLOG(1) << "public key should be 65 bytes";
    return false;
  }

  uint8_t first_byte = *public_key.begin();
  public_key.erase(public_key.begin());
  if (first_byte != 4) {
    VLOG(1) << "First byte of public key should be 4";
    return false;
  }

  EthAddress addr = EthAddress::FromPublicKey(public_key);
  *address = addr.ToChecksumAddress();
  return true;
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

}  // namespace brave_wallet
