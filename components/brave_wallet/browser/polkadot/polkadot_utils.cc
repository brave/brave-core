/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"

#include "base/base64.h"
#include "base/check.h"
#include "base/containers/span.h"
#include "base/containers/span_reader.h"
#include "base/containers/span_writer.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/scrypt_utils.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "crypto/kdf.h"
#include "crypto/process_bound_string.h"
#include "crypto/random.h"

namespace brave_wallet {

namespace {

// Allowed scrypt parameters matching Polkadot.js wallet standards.
struct AllowedScryptParams {
  uint32_t n = 0;
  uint32_t p = 0;
  uint32_t r = 0;
};

// List of allowed scrypt params defined in Polkadot-js:
// https://github.com/polkadot-js/common/blob/fe0886be239526e6c559e98d1099815d4b4f4a7f/packages/util-crypto/src/scrypt/defaults.ts#L10
constexpr AllowedScryptParams kAllowedScryptParams[] = {
    {1 << 13, 10, 8},  // n: 8192, p: 10, r: 8
    {1 << 14, 5, 8},   // n: 16384, p: 5, r: 8
    {1 << 15, 3, 8},   // n: 32768, p: 3, r: 8
    {1 << 15, 1, 8},   // n: 32768, p: 1, r: 8
    {1 << 16, 2, 8},   // n: 65536, p: 2, r: 8
    {1 << 17, 1, 8},   // n: 131072, p: 1, r: 8
};

bool IsAllowedScryptParams(uint32_t n, uint32_t p, uint32_t r) {
  for (const auto& allowed : kAllowedScryptParams) {
    if (allowed.n == n && allowed.p == p && allowed.r == r) {
      return true;
    }
  }
  return false;
}

}  // namespace

std::optional<std::array<uint8_t, kSr25519Pkcs8Size>>
DecodePrivateKeyFromExport(std::string_view json_export,
                           std::string_view password) {
  if (password.empty()) {
    return std::nullopt;
  }

  auto json_dict = base::JSONReader::ReadDict(json_export, 0);
  if (!json_dict) {
    return std::nullopt;
  }

  const std::string* encoded_str = json_dict->FindString("encoded");
  if (!encoded_str) {
    return std::nullopt;
  }

  const base::DictValue* encoding_dict = json_dict->FindDict("encoding");
  if (!encoding_dict) {
    return std::nullopt;
  }

  const base::ListValue* type_list = encoding_dict->FindList("type");
  if (!type_list || type_list->size() != 2) {
    return std::nullopt;
  }

  if (!type_list->contains("scrypt") ||
      !type_list->contains("xsalsa20-poly1305")) {
    return std::nullopt;
  }

  const base::ListValue* content_list = encoding_dict->FindList("content");
  if (!content_list || content_list->size() != 2) {
    return std::nullopt;
  }

  if (!content_list->contains("pkcs8") || !content_list->contains("sr25519")) {
    return std::nullopt;
  }

  const std::string* version = encoding_dict->FindString("version");
  if (!version || *version != "3") {
    return std::nullopt;
  }

  std::string encoded_bytes;
  if (!base::Base64Decode(*encoded_str, &encoded_bytes,
                          base::Base64DecodePolicy::kStrict)) {
    return std::nullopt;
  }

  if (encoded_bytes.size() != kScryptSaltSize + 4 * 3 + kSecretboxAuthTagSize +
                                  kSr25519Pkcs8Size + kSecretboxNonceSize) {
    return std::nullopt;
  }

  base::SpanReader reader(base::as_byte_span(encoded_bytes));

  std::array<uint8_t, kScryptSaltSize> salt = {};
  if (!reader.ReadCopy(salt)) {
    return std::nullopt;
  }

  uint32_t scrypt_n = 0, scrypt_r = 0, scrypt_p = 0;
  if (!reader.ReadU32LittleEndian(scrypt_n) ||
      !reader.ReadU32LittleEndian(scrypt_p) ||
      !reader.ReadU32LittleEndian(scrypt_r)) {
    return std::nullopt;
  }

  if (!IsAllowedScryptParams(scrypt_n, scrypt_p, scrypt_r)) {
    return std::nullopt;
  }

  crypto::kdf::ScryptParams scrypt_params = {
      .cost = scrypt_n,
      .block_size = scrypt_r,
      .parallelization = scrypt_p,
      .max_memory_bytes = 256 * 1024 * 1024,  // 256 MB
  };

  auto scrypt_key = ScryptDeriveKey(password, salt, scrypt_params);
  if (!scrypt_key) {
    return std::nullopt;
  }

  CHECK_EQ(scrypt_key->size(), kScryptKeyBytes);

  std::array<uint8_t, kSecretboxNonceSize> nonce = {};
  if (!reader.ReadCopy(nonce)) {
    return std::nullopt;
  }

  auto encrypted_data = reader.remaining_span();
  CHECK_EQ(encrypted_data.size(), kSr25519Pkcs8Size + kSecretboxAuthTagSize);

  auto decrypt_result = XSalsaPolyDecrypt(
      encrypted_data, nonce, base::span(*scrypt_key).first<kScryptKeyBytes>());
  if (!decrypt_result || decrypt_result->size() != kSr25519Pkcs8Size) {
    return std::nullopt;
  }

  std::array<uint8_t, kSr25519Pkcs8Size> secret_key = {};
  base::span(secret_key).copy_from_nonoverlapping(base::span(*decrypt_result));

  return secret_key;
}

std::optional<std::string> EncodePrivateKeyForExport(
    base::span<const uint8_t, kSr25519Pkcs8Size> pkcs8_key,
    std::string_view address,
    std::string_view password,
    std::span<const uint8_t, kScryptSaltSize> salt,
    std::span<const uint8_t, kSecretboxNonceSize> nonce) {
  if (password.empty()) {
    return std::nullopt;
  }

  SecureVector pkcs8_key_secure(pkcs8_key.begin(), pkcs8_key.end());

  crypto::kdf::ScryptParams scrypt_params = {
      .cost = 32768,
      .block_size = 8,
      .parallelization = 1,
      .max_memory_bytes = 64 * 1024 * 1024,
  };

  auto derived_key = ScryptDeriveKey(password, salt, scrypt_params);
  if (!derived_key.has_value()) {
    return std::nullopt;
  }

  CHECK_EQ(derived_key->size(), kScryptKeyBytes);

  auto encrypt_result = XSalsaPolyEncrypt(
      base::as_byte_span(pkcs8_key_secure),
      base::span(*derived_key).first<kScryptKeyBytes>(), nonce);
  crypto::internal::SecureZeroBuffer(*derived_key);

  if (!encrypt_result.has_value()) {
    return std::nullopt;
  }

  std::vector<uint8_t> encoded_bytes(
      kScryptSaltSize + 4 * 3 + kSecretboxNonceSize + encrypt_result->size(),
      0);
  auto encoded_bytes_span_writer = base::SpanWriter(base::span(encoded_bytes));
  encoded_bytes_span_writer.Write(salt);
  encoded_bytes_span_writer.WriteU32LittleEndian(scrypt_params.cost);
  encoded_bytes_span_writer.WriteU32LittleEndian(scrypt_params.parallelization);
  encoded_bytes_span_writer.WriteU32LittleEndian(scrypt_params.block_size);
  encoded_bytes_span_writer.Write(nonce);
  encoded_bytes_span_writer.Write(*encrypt_result);
  CHECK_EQ(encoded_bytes_span_writer.remaining(), 0u);

  std::string encoded = base::Base64Encode(encoded_bytes);

  base::DictValue json_dict;
  json_dict.Set("encoded", encoded);

  base::DictValue encoding_dict;
  base::ListValue content_list;
  content_list.Append("pkcs8");
  content_list.Append("sr25519");
  encoding_dict.Set("content", std::move(content_list));
  base::ListValue type_list;
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

std::optional<PolkadotAddress> ParsePolkadotAccount(const std::string& input,
                                                    uint16_t ss58_prefix) {
  auto ss58_address = Ss58Address::Decode(input);
  if (ss58_address) {
    if (ss58_address->prefix != ss58_prefix) {
      return std::nullopt;
    }
    return PolkadotAddress{ss58_address->public_key, ss58_prefix};
  }

  // Note: Avoid using PrefixedHexStringToFixed here because it accepts hex
  // strings of the form: 0x123 which is undesireable when being used as a
  // recipient address of funds.
  std::string_view str = input;
  str = base::RemovePrefix(str, "0x").value_or({});
  if (str.empty()) {
    return std::nullopt;
  }

  std::array<uint8_t, kPolkadotSubstrateAccountIdSize> pubkey = {};
  if (base::HexStringToSpan(str, pubkey)) {
    return PolkadotAddress{pubkey, std::nullopt};
  }

  return std::nullopt;
}

std::optional<std::string> PolkadotAddress::ToString() const {
  if (ss58_prefix.has_value()) {
    Ss58Address addr;
    addr.prefix = *ss58_prefix;
    addr.public_key = pubkey;
    return addr.Encode();
  }

  return "0x" + base::HexEncodeLower(pubkey);
}

mojom::uint128Ptr Uint128ToMojom(uint128_t x) {
  return mojom::uint128::New(x >> 64, x & 0xffffffffffffffff);
}

uint128_t MojomToUint128(const mojom::uint128Ptr& x) {
  return (uint128_t{x->high} << 64) | uint128_t{x->low};
}

}  // namespace brave_wallet
