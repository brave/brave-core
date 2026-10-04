/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/ecdh_aes.h"

#include <algorithm>
#include <array>

#include "base/base64.h"
#include "base/check_op.h"
#include "base/containers/span_writer.h"
#include "base/logging.h"
#include "crypto/random.h"
#include "crypto/sha2.h"
#include "third_party/boringssl/src/include/openssl/aead.h"
#include "third_party/boringssl/src/include/openssl/ec.h"
#include "third_party/boringssl/src/include/openssl/ec_key.h"
#include "third_party/boringssl/src/include/openssl/ecdh.h"
#include "third_party/boringssl/src/include/openssl/nid.h"

namespace web_discovery {

namespace {

constexpr size_t kAesKeySize = 16;
constexpr size_t kAesTagLength = 16;
constexpr size_t kIvSize = 12;
constexpr size_t kKeyMaterialSize = 32;
// P-256 field size * 2 + type byte
constexpr size_t kComponentOctSize = 32 * 2 + 1;
// type byte + public component + initialization vector
constexpr size_t kEncodedPubKeyAndIv = 1 + kComponentOctSize + kIvSize;
constexpr uint8_t kP256TypeByte = 0xea;

}  // namespace

AESEncryptResult::AESEncryptResult(std::vector<uint8_t> data,
                                   std::string encoded_public_component_and_iv)
    : data(data),
      encoded_public_component_and_iv(encoded_public_component_and_iv) {}

AESEncryptResult::~AESEncryptResult() = default;

AESEncryptResult::AESEncryptResult(const AESEncryptResult&) = default;
AESEncryptResult& AESEncryptResult::operator=(const AESEncryptResult&) =
    default;

std::optional<AESEncryptResult> DeriveAESKeyAndEncrypt(
    base::span<uint8_t> server_pub_key,
    base::span<uint8_t> data) {
  bssl::UniquePtr<EC_KEY> client_private_key(
      EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));

  if (!client_private_key) {
    VLOG(1) << "Failed to init P-256 curve";
    return std::nullopt;
  }

  bssl::UniquePtr<EC_POINT> server_public_point(EC_POINT_new(EC_group_p256()));
  if (!server_public_point) {
    VLOG(1) << "Failed to init EC public point";
    return std::nullopt;
  }

  if (!EC_POINT_oct2point(EC_group_p256(), server_public_point.get(),
                          server_pub_key.data(), server_pub_key.size(),
                          nullptr)) {
    VLOG(1) << "Failed to load server public key data into EC point";
    return std::nullopt;
  }

  if (!EC_KEY_generate_key(client_private_key.get())) {
    VLOG(1) << "Failed to generate client EC key";
    return std::nullopt;
  }

  std::array<uint8_t, kKeyMaterialSize> shared_key_material;
  if (!ECDH_compute_key(shared_key_material.data(), shared_key_material.size(),
                        server_public_point.get(), client_private_key.get(),
                        nullptr)) {
    VLOG(1) << "Failed to set derive key via ECDH";
    return std::nullopt;
  }

  auto key_material_hash = crypto::SHA256Hash(shared_key_material);

  auto aes_key = base::span<uint8_t>(key_material_hash).first<kAesKeySize>();
  auto* algo = EVP_aead_aes_128_gcm();

  bssl::ScopedEVP_AEAD_CTX ctx;
  if (!EVP_AEAD_CTX_init(ctx.get(), algo, aes_key.data(), aes_key.size(),
                         kAesTagLength, nullptr)) {
    VLOG(1) << "Failed to init AEAD context";
    return std::nullopt;
  }

  size_t len;
  std::array<uint8_t, kIvSize> iv;

  crypto::RandBytes(iv);

  std::vector<uint8_t> output(data.size() + EVP_AEAD_max_overhead(algo));
  if (!EVP_AEAD_CTX_seal(ctx.get(), output.data(), &len, output.size(),
                         iv.data(), iv.size(), data.data(), data.size(),
                         nullptr, 0)) {
    VLOG(1) << "Failed to encrypt via AES";
    return std::nullopt;
  }

  output.resize(len);

  std::array<uint8_t, kEncodedPubKeyAndIv> public_component_and_iv;
  auto public_component_and_iv_writer =
      base::SpanWriter(base::span(public_component_and_iv));
  public_component_and_iv_writer.Write(kP256TypeByte);

  auto component = public_component_and_iv_writer.Skip(kComponentOctSize);

  if (!EC_POINT_point2oct(EC_group_p256(),
                          EC_KEY_get0_public_key(client_private_key.get()),
                          POINT_CONVERSION_UNCOMPRESSED, component->data(),
                          kComponentOctSize, nullptr)) {
    VLOG(1) << "Failed to export EC public point/key";
    return std::nullopt;
  }

  public_component_and_iv_writer.Write(iv);
  CHECK_EQ(public_component_and_iv_writer.remaining(), 0u);

  return std::make_optional<AESEncryptResult>(
      output, base::Base64Encode(public_component_and_iv));
}

}  // namespace web_discovery
