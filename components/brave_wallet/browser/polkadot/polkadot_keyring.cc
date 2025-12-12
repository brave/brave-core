/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_keyring.h"

#include "base/base64.h"
#include "base/check_is_test.h"
#include "base/containers/span.h"
#include "base/json/json_writer.h"
#include "base/numerics/byte_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/vendor/bat-native-tweetnacl/tweetnacl.h"
#include "crypto/kdf.h"
#include "crypto/random.h"

namespace brave_wallet {

namespace {

// Address prefixes based on network:
// https://wiki.polkadot.com/learn/learn-account-advanced/.
constexpr uint8_t kPolkadotPrefix = 0u;
// 42 relates to general Substrate address.
constexpr uint8_t kWestendPrefix = 42u;
constexpr uint8_t kSubstratePrefix = 42u;

inline constexpr char const kPolkadotTestnet[] =
    "\x1c"
    "westend";

inline constexpr char const kPolkadotMainnet[] =
    "\x20"
    "polkadot";

}  // namespace

PolkadotKeyring::PolkadotKeyring(
    base::span<const uint8_t, kPolkadotSeedSize> seed,
    mojom::KeyringId keyring_id)
    : root_account_key_(HDKeySr25519::GenerateFromSeed(seed)),
      keyring_id_(keyring_id) {
  // Can be useful to remember:
  // https://wiki.polkadot.com/learn/learn-account-advanced/#derivation-paths

  CHECK(IsPolkadotKeyring(keyring_id));

  if (IsTestnet()) {
    root_account_key_ =
        root_account_key_.DeriveHard(base::as_byte_span(kPolkadotTestnet));
  } else {
    root_account_key_ =
        root_account_key_.DeriveHard(base::as_byte_span(kPolkadotMainnet));
  }
}

PolkadotKeyring::~PolkadotKeyring() = default;

bool PolkadotKeyring::IsTestnet() const {
  return keyring_id_ == mojom::KeyringId::kPolkadotTestnet;
}

std::array<uint8_t, kSr25519PublicKeySize> PolkadotKeyring::GetPublicKey(
    uint32_t account_index) {
  auto const& keypair = EnsureKeyPair(account_index);
  return keypair.GetPublicKey();
}

std::string PolkadotKeyring::GetAddress(uint32_t account_index,
                                        uint16_t prefix) {
  auto& keypair = EnsureKeyPair(account_index);

  Ss58Address addr;
  addr.prefix = prefix;
  base::span(addr.public_key)
      .copy_from_nonoverlapping(
          base::span<uint8_t const>(keypair.GetPublicKey()));

  return addr.Encode().value();
}

std::array<uint8_t, kSr25519SignatureSize> PolkadotKeyring::SignMessage(
    base::span<const uint8_t> message,
    uint32_t account_index) {
  auto const& keypair = EnsureKeyPair(account_index);
  return keypair.SignMessage(message);
}

[[nodiscard]] bool PolkadotKeyring::VerifyMessage(
    base::span<const uint8_t, kSr25519SignatureSize> signature,
    base::span<const uint8_t> message,
    uint32_t account_index) {
  auto const& keypair = EnsureKeyPair(account_index);
  return keypair.VerifyMessage(signature, message);
}

HDKeySr25519& PolkadotKeyring::EnsureKeyPair(uint32_t account_index) {
  auto pos = secondary_keys_.find(account_index);
  if (pos == secondary_keys_.end()) {
    auto [it, inserted] = secondary_keys_.emplace(
        account_index,
        root_account_key_.DeriveHard(base::byte_span_from_ref(account_index)));
    pos = it;
  }
  return pos->second;
}

std::optional<std::string> PolkadotKeyring::AddNewHDAccount(uint32_t index) {
  return GetAddress(index, IsTestnet() ? kWestendPrefix : kPolkadotPrefix);
}

void PolkadotKeyring::SetRandBytesForTesting(  // IN-TEST
    const std::vector<uint8_t>& seed_bytes,
    const std::vector<uint8_t>& nonce_bytes) {
  CHECK_IS_TEST();
  rand_seed_bytes_for_testing_ = seed_bytes;
  rand_nonce_bytes_for_testing_ = nonce_bytes;
}

// Creates JSON to export Polkadot account info in the proper format.
// At this time password is reused to encrypt encoded data.
std::optional<std::string> PolkadotKeyring::EncodePrivateKeyForExport(
    uint32_t account_index,
    const std::string& password) {
  if (password.empty()) {
    return std::nullopt;
  }

  auto& keypair = EnsureKeyPair(account_index);
  auto seed = keypair.GetExportKeyPkcs8();

  // Get the address for this account.
  std::string address = GetAddress(account_index, kSubstratePrefix);

  // Generate a random salt for scrypt.
  std::vector<uint8_t> salt(32);
  if (rand_seed_bytes_for_testing_) {
    CHECK_IS_TEST();
    salt = rand_seed_bytes_for_testing_.value();
  } else {
    crypto::RandBytes(salt);
  }

  // Derive encryption key from password using scrypt
  // Substrate/Polkadot standard parameters: n=32768, r=8, p=1.
  std::array<uint8_t, crypto_secretbox_KEYBYTES> derived_key;
  crypto::kdf::ScryptParams scrypt_params = {
      .cost = 32768,                         // n
      .block_size = 8,                       // r
      .parallelization = 1,                  // p
      .max_memory_bytes = 64 * 1024 * 1024,  // 64 MB
  };

  if (!crypto::kdf::DeriveKeyScryptNoCheck(
          scrypt_params, base::as_byte_span(password),
          base::span<const uint8_t>(salt), derived_key)) {
    return std::nullopt;
  }

  // Generate a random nonce for xsalsa20-poly1305.
  std::vector<uint8_t> nonce(crypto_secretbox_NONCEBYTES);
  if (rand_nonce_bytes_for_testing_) {
    CHECK_IS_TEST();
    nonce = rand_nonce_bytes_for_testing_.value();
  } else {
    crypto::RandBytes(nonce);
  }

  // Encrypt the seed using NaCl secretbox (xsalsa20-poly1305).
  std::vector<uint8_t> plaintext(seed.begin(), seed.end());
  std::vector<uint8_t> padded_plaintext(crypto_secretbox_ZEROBYTES +
                                        plaintext.size());
  std::vector<uint8_t> ciphertext(padded_plaintext.size());

  // Pad plaintext with zeros (NaCl secretbox requirement).
  for (size_t i = 0; i < plaintext.size(); ++i) {
    padded_plaintext[i + crypto_secretbox_ZEROBYTES] = plaintext[i];
  }

  if (crypto_secretbox(ciphertext.data(), padded_plaintext.data(),
                       padded_plaintext.size(), nonce.data(),
                       derived_key.data()) != 0) {
    return std::nullopt;
  }

  // Extract the actual ciphertext (skip the zero bytes prefix).
  std::vector<uint8_t> encrypted_data(
      ciphertext.begin() + crypto_secretbox_BOXZEROBYTES, ciphertext.end());

  // Encode in polkadot-js format: scryptToU8a(salt, params) + nonce + encrypted
  // scryptToU8a encodes: salt (32 bytes) + n (4 bytes LE) + r (4 bytes LE) + p
  // (4 bytes LE).
  // https://github.com/polkadot-js/common/blob/bf63a0ebf655312f54aa37350d244df3d05e4e32/packages/keyring/src/pair/encode.ts#L14
  std::vector<uint8_t> encoded_bytes;
  encoded_bytes.reserve(32 + 4 + 4 + 4 + nonce.size() + encrypted_data.size());

  // Add salt (32 bytes)
  encoded_bytes.insert(encoded_bytes.end(), salt.begin(), salt.end());

  // Add scrypt parameters as 32-bit little-endian integers.
  auto n_bytes = base::U32ToLittleEndian(scrypt_params.cost);
  auto p_bytes = base::U32ToLittleEndian(scrypt_params.parallelization);
  auto r_bytes = base::U32ToLittleEndian(scrypt_params.block_size);

  encoded_bytes.insert(encoded_bytes.end(), n_bytes.begin(), n_bytes.end());
  encoded_bytes.insert(encoded_bytes.end(), p_bytes.begin(), p_bytes.end());
  encoded_bytes.insert(encoded_bytes.end(), r_bytes.begin(), r_bytes.end());

  // Add nonce.
  encoded_bytes.insert(encoded_bytes.end(), nonce.begin(), nonce.end());

  // Add encrypted data.
  encoded_bytes.insert(encoded_bytes.end(), encrypted_data.begin(),
                       encrypted_data.end());

  // Encode as base64 for the "encoded" field.
  std::string encoded = base::Base64Encode(encoded_bytes);

  // Build the JSON structure.
  base::Value::Dict json_dict;
  json_dict.Set("encoded", encoded);

  base::Value::Dict encoding_dict;
  base::Value::List content_list;
  content_list.Append("pkcs8");
  content_list.Append("sr25519");
  encoding_dict.Set("content", std::move(content_list));
  base::Value::List type_list;
  type_list.Append("scrypt");
  type_list.Append("xsalsa20-poly1305");
  encoding_dict.Set("type", std::move(type_list));
  encoding_dict.Set("version", "3");

  json_dict.Set("encoding", std::move(encoding_dict));

  json_dict.Set("address", address);

  std::string json_string;
  if (!base::JSONWriter::Write(json_dict, &json_string)) {
    return std::nullopt;
  }

  return json_string;
}

}  // namespace brave_wallet
