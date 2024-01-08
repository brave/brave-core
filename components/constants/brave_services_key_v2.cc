// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/constants/brave_services_key_v2.h"

#include <vector>
#include "base/logging.h"

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "brave/components/constants/buildflags.h"
#include "crypto/hmac.h"
#include "crypto/sha2.h"

namespace constants {

namespace {

std::optional<std::string> GetServiceKey(Service service) {
  if (service == Service::kAIChat) {
    return BUILDFLAG(AI_CHAT_SERVICE_KEY);
  }

  return std::nullopt;
}

}  // namespace

std::optional<std::pair<std::string, std::string>> GetBraveServicesV2Headers(
    const std::string payload,
    Service service) {
  // Calculate the SHA-256 hash of the payload, then encode it in base64 format
  // to generate the digest header. This also forms the signature string that
  // needs to be signed.
  std::string request_digest_base64;
  base::Base64Encode(crypto::SHA256HashString(payload), &request_digest_base64);
  const std::string digest_header =
      base::StringPrintf("SHA-256=%s", request_digest_base64.c_str());
  const std::string signature_string =
      base::StringPrintf("digest: %s", digest_header.c_str());

  // Calculate the HMAC-SHA256 hash of the signature string using the specified
  // service key.
  auto service_key = GetServiceKey(service);
  if (!service_key) {
    return absl::nullopt;
  }

  crypto::HMAC hmac(crypto::HMAC::SHA256);
  const size_t signature_digest_length = hmac.DigestLength();
  std::vector<uint8_t> signature_digest(signature_digest_length);
  const bool success = hmac.Init(*service_key) &&
                       hmac.Sign(signature_string, &signature_digest[0],
                                 signature_digest.size());
  if (!success) {
    return std::nullopt;
  }

  // Use the HMAC-SHA256 to create the authorization header.
  std::string signature_digest_base64;
  base::Base64Encode(
      std::string(signature_digest.begin(), signature_digest.end()),
      &signature_digest_base64);
  const std::string authorization_header = base::StringPrintf(
      "Signature "
      "keyId=\"%s\",algorithm=\"hs2019\",headers="
      "\"digest\",signature=\"%s\"",
      BUILDFLAG(KEY_ID), signature_digest_base64.c_str());

  return std::make_pair(digest_header, authorization_header);
}

}  // namespace constants
