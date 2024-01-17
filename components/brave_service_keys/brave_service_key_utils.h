// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SERVICE_KEYS_BRAVE_SERVICE_KEY_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_SERVICE_KEYS_BRAVE_SERVICE_KEY_UTILS_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "url/gurl.h"

namespace brave_service_keys {

// Calculates the SHA-256 hash of the supplied payload and returns a pair
// comprising of the digest header field, and header value in the format
// "SHA-256=<base64_encoded_hash>".
std::pair<std::string, std::string> GetDigestHeader(const std::string& payload);

// Generates the the string to be signed over and included in the authorization
// header. See
// https://datatracker.ietf.org/doc/html/draft-cavage-http-signatures-08#section-2.3:w
std::pair<std::string, std::string> CreateSignatureString(
    const base::flat_map<std::string, std::string>& headers,
    const GURL& url,
    const std::string& method,
    const std::vector<std::string>& headers_to_sign);

// Generates an authorization header field and value pair using the provided
// service key to sign over specified headers.
std::optional<std::pair<std::string, std::string>> GetAuthorizationHeader(
    const std::string& service_key,
    const base::flat_map<std::string, std::string>& headers,
    const GURL& url,
    const std::string& method,
    const std::vector<std::string>& headers_to_sign);

}  // namespace brave_service_keys

#endif  // BRAVE_COMPONENTS_BRAVE_SERVICE_KEYS_BRAVE_SERVICE_KEY_UTILS_H_
