// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_service_keys/service_key_utils.h"

#include <vector>

#include "base/base64.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_service_keys/buildflags.h"
#include "crypto/hmac.h"
#include "crypto/sha2.h"

namespace brave_service_keys {

namespace {

constexpr char kDigest[] = "digest";
constexpr char kAuthorization[] = "authorization";

}  // namespace

std::pair<std::string, std::string> GetDigestHeader(
    const std::string& payload) {
  const std::string value = base::StrCat(
      {"SHA-256=", base::Base64Encode(crypto::SHA256HashString(payload))});
  return std::make_pair(kDigest, value);
}

std::pair<std::string, std::string> CreateSignatureString(
    const std::vector<std::pair<std::string, std::string>>& headers) {
  std::string header_names;
  std::string signature_string;
  for (auto& [key, value] : headers) {
    if (!header_names.empty()) {
      header_names += " ";
      signature_string += "\n";
    }
    header_names += key;
    signature_string += key + ": " + value;
  }
  return {header_names, signature_string};
}

std::optional<std::pair<std::string, std::string>> GetAuthorizationHeader(
    const std::string& service_key,
    const std::vector<std::pair<std::string, std::string>>& headers) {
  auto [header_names, signature_string] = CreateSignatureString(headers);

  // Create the signature using the service_key.
  crypto::HMAC hmac(crypto::HMAC::SHA256);
  const size_t signature_digest_length = hmac.DigestLength();
  std::vector<uint8_t> signature_digest(signature_digest_length);
  const bool success = hmac.Init(service_key) &&
                       hmac.Sign(signature_string, &signature_digest[0],
                                 signature_digest.size());
  if (!success) {
    return std::nullopt;
  }

  // Create the authorization header.
  std::string signature_digest_base64;
  base::Base64Encode(
      std::string(signature_digest.begin(), signature_digest.end()),
      &signature_digest_base64);

  const std::string value = base::StrCat(
      {"Signature "
       "keyId=\"",
       BUILDFLAG(KEY_ID), "\",algorithm=\"hs2019\",headers=\"", header_names,
       "\",signature=\"", signature_digest_base64, "\""});

  return std::make_pair(kAuthorization, value);
}

}  // namespace brave_service_keys
