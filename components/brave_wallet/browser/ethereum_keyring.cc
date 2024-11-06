/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/ethereum_keyring.h"

#include <optional>

#include "base/base64.h"
#include "base/containers/extend.h"
#include "base/containers/span.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace brave_wallet {

namespace {

// Get the 32 byte message hash
KeccakHashArray GetMessageHash(base::span<const uint8_t> message) {
  std::string prefix =
      "\x19"
      "Ethereum Signed Message:\n" +
      base::NumberToString(message.size());
  std::vector<uint8_t> hash_input(prefix.begin(), prefix.end());
  base::Extend(hash_input, message);
  return KeccakHash(hash_input);
}

}  // namespace

EthereumKeyring::EthereumKeyring(base::span<const uint8_t> seed)
    : Secp256k1HDKeyring(seed, GetRootPath(mojom::KeyringId::kDefault)) {}

// static
std::optional<std::string> EthereumKeyring::RecoverAddress(
    base::span<const uint8_t> message,
    base::span<const uint8_t, kRecoverableSignatureSize> signature) {
  uint8_t v = signature.back();
  if (v < 27) {
    VLOG(1) << "v should be >= 27";
    return std::nullopt;
  }

  // v = chain_id ? recid + chain_id * 2 + 35 : recid + 27;
  // So recid = v - 27 when chain_id is 0
  uint8_t recid = v - 27;
  auto hash = GetMessageHash(message);

  // Public keys (in scripts) are given as 04 <x> <y> where x and y are 32
  // byte big-endian integers representing the coordinates of a point on the
  // curve or in compressed form given as <sign> <x> where <sign> is 0x02 if
  // y is even and 0x03 if y is odd.
  HDKey key;
  std::vector<uint8_t> public_key = key.RecoverCompact(
      false, hash, signature.first<kCompactSignatureSize>(), recid);
  if (public_key.size() != 65) {
    VLOG(1) << "public key should be 65 bytes";
    return std::nullopt;
  }

  uint8_t first_byte = *public_key.begin();
  public_key.erase(public_key.begin());
  if (first_byte != 4) {
    VLOG(1) << "First byte of public key should be 4";
    return std::nullopt;
  }

  EthAddress addr = EthAddress::FromPublicKey(public_key);
  return addr.ToChecksumAddress();
}

std::vector<uint8_t> EthereumKeyring::SignMessage(
    const std::string& address,
    base::span<const uint8_t> message,
    uint256_t chain_id,
    bool is_eip712) {
  HDKey* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key) {
    return std::vector<uint8_t>();
  }

  std::array<uint8_t, kSecp256k1SignMsgSize> hashed_message;
  if (!is_eip712) {
    hashed_message = GetMessageHash(message);
  } else {
    // eip712 hash is Keccak
    if (message.size() != 32) {
      return std::vector<uint8_t>();
    }

    base::span(hashed_message).copy_from(message);
  }

  int recid;
  auto signature = hd_key->SignCompact(hashed_message, &recid);
  if (!signature) {
    return std::vector<uint8_t>();
  }

  std::vector<uint8_t> eth_signature(signature->begin(), signature->end());
  uint8_t v =
      static_cast<uint8_t>(chain_id ? recid + chain_id * 2 + 35 : recid + 27);
  eth_signature.push_back(v);

  return eth_signature;
}

void EthereumKeyring::SignTransaction(const std::string& address,
                                      EthTransaction* tx,
                                      uint256_t chain_id) {
  HDKey* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key || !tx) {
    return;
  }

  int recid = 0;
  auto signature =
      hd_key->SignCompact(tx->GetHashedMessageToSign(chain_id), &recid);
  if (!signature) {
    return;
  }
  tx->ProcessSignature(*signature, recid, chain_id);
}

std::string EthereumKeyring::GetAddressInternal(const HDKey& hd_key) const {
  const std::vector<uint8_t> public_key = hd_key.GetUncompressedPublicKey();
  // trim the header byte 0x04
  const std::vector<uint8_t> pubkey_no_header(public_key.begin() + 1,
                                              public_key.end());
  EthAddress addr = EthAddress::FromPublicKey(pubkey_no_header);

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

std::string EthereumKeyring::EncodePrivateKeyForExport(
    const std::string& address) {
  HDKey* hd_key = GetHDKeyFromAddress(address);
  if (!hd_key) {
    return std::string();
  }

  return base::ToLowerASCII(base::HexEncode(hd_key->GetPrivateKeyBytes()));
}

std::unique_ptr<HDKey> EthereumKeyring::DeriveAccount(uint32_t index) const {
  // m/44'/60'/0'/0/{index}
  return root_->DeriveNormalChild(index);
}

}  // namespace brave_wallet
