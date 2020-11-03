/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/security/security_util.h"

#include <openssl/digest.h>
#include <openssl/hkdf.h>
#include <openssl/sha.h>

#include <algorithm>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "tweetnacl.h"  // NOLINT
#include "wrapper.hpp"
#include "bat/ads/internal/confirmations/confirmation_info.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/create_confirmation_util.h"

namespace ads {
namespace security {

using challenge_bypass_ristretto::UnblindedToken;
using challenge_bypass_ristretto::VerificationKey;
using challenge_bypass_ristretto::VerificationSignature;

namespace {

const int kHKDFSeedLength = 32;

const uint8_t kHKDFSalt[] = {
  126, 244,  99, 158,  51,  68, 253,  80,
  133, 183,  51, 180,  77,  62,  74, 252,
   62, 106,  96, 125, 241, 110, 134,  87,
  190, 208, 158,  84, 125,  69, 246, 207,
  162, 247, 107, 172,  37,  34,  53, 246,
  105,  20, 215,   5, 248, 154, 179, 191,
   46,  17,   6,  72, 210,  91,  10, 169,
  145, 248,  22, 147, 117,  24, 105,  12
};

std::vector<uint8_t> GetHKDF(
    const std::string& secret) {
  if (secret.empty()) {
    return {};
  }

  std::vector<uint8_t> raw_secret;
  raw_secret.assign(secret.begin(), secret.end());

  std::vector<uint8_t> derived_key(kHKDFSeedLength);

  const uint8_t info[] = { 0 };

  const int result = HKDF(&derived_key.front(), kHKDFSeedLength, EVP_sha512(),
      &raw_secret.front(), raw_secret.size(), kHKDFSalt, base::size(kHKDFSalt),
          info, sizeof(info) / sizeof(info[0]));

  if (result == 0) {
    return {};
  }

  return derived_key;
}

bool GenerateKeyPair(
    const std::vector<uint8_t>& seed,
    std::vector<uint8_t>* public_key,
    std::vector<uint8_t>* secret_key) {
  if (seed.empty() || !public_key || !secret_key) {
    return false;
  }

  public_key->resize(crypto_sign_PUBLICKEYBYTES);

  *secret_key = seed;
  secret_key->resize(crypto_sign_SECRETKEYBYTES);

  crypto_sign_keypair(&public_key->front(), &secret_key->front(), 1);

  if (public_key->empty() || secret_key->empty()) {
    return false;
  }

  return true;
}

}  // namespace

std::vector<uint8_t> GenerateSecretKeyFromSeed(
    const std::string& seed_base64) {
  std::string seed;
  if (!base::Base64Decode(seed_base64, &seed)) {
    return {};
  }

  const std::vector<uint8_t> derived_key = GetHKDF(seed);

  std::vector<uint8_t> public_key;
  std::vector<uint8_t> secret_key;
  if (!GenerateKeyPair(derived_key, &public_key, &secret_key)) {
    return {};
  }

  return secret_key;
}

std::string Sign(
    const std::map<std::string, std::string>& headers,
    const std::string& key_id,
    const std::string& secret_key) {
  if (headers.empty() || key_id.empty() || secret_key.empty()) {
    return "";
  }

  std::string concatenated_header = "";
  std::string concatenated_message = "";

  unsigned int index = 0;
  for (const auto& header : headers) {
    if (index != 0) {
      concatenated_header += " ";
      concatenated_message += "\n";
    }

    concatenated_header += header.first;
    concatenated_message += header.first + ": " + header.second;

    index++;
  }

  std::vector<uint8_t> signed_message(crypto_sign_BYTES +
      concatenated_message.length());

  std::vector<uint8_t> raw_secret_key;
  if (!base::HexStringToBytes(secret_key, &raw_secret_key)) {
    return "";
  }

  // Resolving the following linter error breaks the build on Windows
  unsigned long long signed_message_length = 0;  // NOLINT
  crypto_sign(&signed_message.front(), &signed_message_length,
      reinterpret_cast<const unsigned char*>(concatenated_message.c_str()),
          concatenated_message.length(), &raw_secret_key.front());

  std::vector<uint8_t> signature(crypto_sign_BYTES);
  std::copy(signed_message.begin(), signed_message.begin() +
      crypto_sign_BYTES, signature.begin());

  return "keyId=\"" + key_id + "\",algorithm=\"" + crypto_sign_PRIMITIVE +
      "\",headers=\"" + concatenated_header + "\",signature=\"" +
          base::Base64Encode(signature) + "\"";
}

std::vector<uint8_t> Sha256Hash(
    const std::string& value) {
  if (value.empty()) {
    return {};
  }

  std::vector<uint8_t> sha256(SHA256_DIGEST_LENGTH);
  SHA256((uint8_t*)value.c_str(), value.length(), &sha256.front());
  return sha256;
}

bool Verify(
    const ConfirmationInfo& confirmation) {
  std::string credential;
  base::Base64Decode(confirmation.credential, &credential);

  base::Optional<base::Value> value = base::JSONReader::Read(credential);
  if (!value || !value->is_dict()) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return false;
  }

  const std::string* signature = dictionary->FindStringKey("signature");
  if (!signature) {
    return false;
  }

  VerificationSignature verification_signature =
      VerificationSignature::decode_base64(*signature);

  const std::string payload = CreateConfirmationRequestDTO(confirmation);

  UnblindedToken unblinded_token = confirmation.unblinded_token.value;
  VerificationKey verification_key = unblinded_token.derive_verification_key();

  return verification_key.verify(verification_signature, payload);
}

}  // namespace security
}  // namespace ads
