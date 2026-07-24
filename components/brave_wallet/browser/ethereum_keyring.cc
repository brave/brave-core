/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/ethereum_keyring.h"

#include <array>
#include <optional>

#include "base/base64.h"
#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/internal/hd_key_common.h"
#include "brave/components/brave_wallet/browser/internal/secp256k1_signature.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

namespace {

std::unique_ptr<HDKey> ConstructAccountsRootKey(
    base::span<const uint8_t> seed) {
  auto result = HDKey::GenerateFromSeed(seed);
  if (!result) {
    return nullptr;
  }

  // m/44'/60'/0'/0
  return result->DeriveChildFromPath({DerivationIndex::Hardened(44),  //
                                      DerivationIndex::Hardened(60),
                                      DerivationIndex::Hardened(0),
                                      DerivationIndex::Normal(0)});
}

}  // namespace

EthereumKeyring::EthereumKeyring(
    base::span<const uint8_t> seed,
    base::RepeatingCallback<bool(const std::string&)> is_address_allowed)
    : Secp256k1HDKeyring(std::move(is_address_allowed)) {
  accounts_root_ = ConstructAccountsRootKey(seed);
}

// static
std::optional<std::string> EthereumKeyring::RecoverAddress(
    const KeccakHashArray& message_hash,
    const Secp256k1Signature& signature) {
  // if (v_bytes.size() != 1) {
  //   return std::nullopt;
  // }

  // uint8_t v = v_bytes.front();
  // if (v < 27) {
  //   return std::nullopt;
  // }

  // // v = chain_id ? recid + chain_id * 2 + 35 : recid + 27;
  // // So recid = v - 27 when chain_id is 0
  // auto signature = Secp256k1Signature::CreateFromPayload(rs_bytes, v - 27);
  // if (!signature) {
  //   return std::nullopt;
  // }

  // Public keys (in scripts) are given as 04 <x> <y> where x and y are 32
  // byte big-endian integers representing the coordinates of a point on the
  // curve or in compressed form given as <sign> <x> where <sign> is 0x02 if
  // y is even and 0x03 if y is odd.
  HDKey key;
  auto public_key = key.RecoverCompact(false, message_hash, signature);
  if (!public_key || public_key->size() != 65) {
    return std::nullopt;
  }

  uint8_t first_byte = public_key->front();
  if (first_byte != 4) {
    return std::nullopt;
  }

  EthAddress addr = EthAddress::FromPublicKey(
      base::span(*public_key).last<kEthPublicKeyLength>());
  return addr.ToChecksumAddress();
}

std::optional<Secp256k1Signature> EthereumKeyring::SignMessage(
    const std::string& address,
    const KeccakHashArray& message_hash) {
  HDKey* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key) {
    return std::nullopt;
  }

  // std::array<uint8_t, kSecp256k1SignMsgSize> message_hash = {};
  // if (!is_eip712) {
  //   message_hash = GetMessageHash(message);
  // } else {
  //   // eip712 hash is Keccak
  //   if (message.size() != 32) {
  //     return std::nullopt;
  //   }

  //   base::span(message_hash).copy_from(message);
  // }

  return hd_key->SignCompact(message_hash);

  // MM does not use chain_id for message signing. Just recovery `id + 27` as
  // last signature byte.
  // https://github.com/MetaMask/eth-sig-util/blob/0832d49b7c2f6d48d22a4496faee3e393081d1ec/src/personal-sign.ts#L43-L44
  // https://github.com/ethereumjs/ethereumjs-monorepo/blob/460368319c57d0bad1683f718a21f557d9e1eec5/packages/util/src/signature.ts#L37-L53
  // auto result = base::ToVector(signature->bytes());
  // CHECK(result.size());
  // result.back() += 27;
  // return result;
}

// void EthereumKeyring::SignTransaction(const std::string& address,
//                                       EthTransaction* tx,
//                                       uint256_t chain_id) {
//   HDKey* hd_key = GetHDKeyFromAddress(address);
//   if (!hd_key || !tx) {
//     return;
//   }

auto signature = hd_key->SignCompact(tx->GetHashedMessageToSign(chain_id));
if (!signature) {
  return;
}
tx->ProcessSignature(*signature, chain_id);
}

std::string EthereumKeyring::GetAddressInternal(const HDKey& hd_key) const {
  const std::vector<uint8_t> public_key = hd_key.GetUncompressedPublicKey();
  // trim the header byte 0x04
  const std::vector<uint8_t> pubkey_no_header(public_key.begin() + 1,
                                              public_key.end());
  auto pubkey =
      base::span(pubkey_no_header).to_fixed_extent<kEthPublicKeyLength>();
  CHECK(pubkey);
  auto addr = EthAddress::FromPublicKey(*pubkey);

  // TODO(darkdh): chain id op code
  return addr.ToChecksumAddress();
}

bool EthereumKeyring::GetPublicKeyFromX25519_XSalsa20_Poly1305(
    const std::string& address,
    std::string* key) {
  HDKey* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key) {
    return false;
  }
  const std::vector<uint8_t> public_key =
      hd_key->GetPublicKeyFromX25519_XSalsa20_Poly1305();
  if (public_key.empty()) {
    return false;
  }
  *key = base::Base64Encode(public_key);
  return true;
}

std::optional<std::vector<uint8_t>>
EthereumKeyring::DecryptCipherFromX25519_XSalsa20_Poly1305(
    const std::string& version,
    base::span<const uint8_t> nonce,
    base::span<const uint8_t> ephemeral_public_key,
    base::span<const uint8_t> ciphertext,
    const std::string& address) {
  HDKey* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key) {
    return std::nullopt;
  }
  return hd_key->DecryptCipherFromX25519_XSalsa20_Poly1305(
      version, nonce, ephemeral_public_key, ciphertext);
}

std::optional<std::string> EthereumKeyring::GetDiscoveryAddress(
    size_t index) const {
  if (auto key = DeriveAccount(index)) {
    return GetAddressInternal(*key);
  }
  return std::nullopt;
}

std::optional<std::string> EthereumKeyring::EncodePrivateKeyForExport(
    const std::string& address) {
  HDKey* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key) {
    return std::nullopt;
  }

  return base::HexEncodeLower(hd_key->GetPrivateKeyBytes());
}

std::unique_ptr<HDKey> EthereumKeyring::DeriveAccount(uint32_t index) const {
  // m/44'/60'/0'/0/{index}
  return accounts_root_->DeriveChild(DerivationIndex::Normal(index));
}

std::vector<std::string> EthereumKeyring::GetImportedAccountsForTesting()
    const {
  std::vector<std::string> addresses;
  for (auto& acc : imported_accounts_) {
    addresses.push_back(GetAddressInternal(*acc.second));
  }
  return addresses;
}

}  // namespace brave_wallet
