// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_service_keys/brave_service_key_utils.h"

#include <vector>

#include "base/base64.h"
#include "base/notreached.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_service_keys/buildflags.h"
#include "crypto/hmac.h"
#include "crypto/sha2.h"
#include "net/http/http_auth_scheme.h"
#include "net/http/http_request_headers.h"

namespace brave_service_keys {

namespace {

constexpr char kRequestTargetHeader[] = "(request-target)";

}  // namespace

std::pair<std::string, std::string> GetDigestHeader(
    const std::string& payload) {
  const std::string value = base::StrCat(
      {"SHA-256=", base::Base64Encode(crypto::SHA256HashString(payload))});
  return std::make_pair(net::kDigestAuthScheme, value);
}

std::pair<std::string, std::string> CreateSignatureString(
    const base::flat_map<std::string, std::string>& headers,
    const GURL& url,
    const std::string& method,
    const std::vector<std::string>& headers_to_sign) {
  std::string header_names;
  std::string signature_string;

  for (const auto& header_to_sign : headers_to_sign) {
    // Prepend some padding / newlines if this isn't the first
    // header to sign
    if (!header_names.empty()) {
      header_names.append(" ");
      signature_string.append("\n");
    }
    header_names.append(header_to_sign);

    // Handle the special case header (request-target) by constructing
    // the value instead of getting it from headers.
    if (header_to_sign == kRequestTargetHeader) {
      signature_string.append(
          base::StrCat({kRequestTargetHeader, ": ", base::ToLowerASCII(method),
                        " ", url.PathForRequest()}));
      continue;
    }

    // For all the headers to sign, we expect their values to be be in the
    // headers flat_map and use the value there to add to the signature string.
    auto header = headers.find(header_to_sign);
    CHECK(header != headers.end())
        << "Can't sign over non-existent header " << header_to_sign;
    signature_string.append(
        base::StrCat({header_to_sign, ": ", header->second}));
  }

  return std::make_pair(header_names, signature_string);
}

std::optional<std::pair<std::string, std::string>> GetAuthorizationHeader(
    const std::string& service_key,
    const base::flat_map<std::string, std::string>& headers,
    const GURL& url,
    const std::string& method,
    const std::vector<std::string>& headers_to_sign) {
  CHECK(url.is_valid());
  auto [header_names, signature_string] =
      CreateSignatureString(headers, url, method, headers_to_sign);

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

  const std::string value = base::StrCat(
      {"Signature keyId=\"", BUILDFLAG(BRAVE_SERVICES_KEY_ID),
       "\",algorithm=\"hs2019\",headers=\"", header_names, "\",signature=\"",
       base::Base64Encode(signature_digest), "\""});

  return std::make_pair(net::HttpRequestHeaders::kAuthorization, value);
}

}  // namespace brave_service_keys
