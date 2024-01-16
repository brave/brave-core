// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SERVICE_KEYS_SERVICE_KEY_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_SERVICE_KEYS_SERVICE_KEY_UTILS_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "url/gurl.h"

namespace brave_service_keys {

std::pair<std::string, std::string> GetDigestHeader(const std::string& payload);

std::pair<std::string, std::string> CreateSignatureString(
    base::flat_map<std::string, std::string> headers,
    const GURL& url,
    const std::string& method,
    const std::vector<std::string>& headers_to_sign);

std::optional<std::pair<std::string, std::string>> GetAuthorizationHeader(
    const std::string& service_key,
    const base::flat_map<std::string, std::string>& headers,
    const GURL& url,
    const std::string& method,
    const std::vector<std::string>& headers_to_sign);

}  // namespace brave_service_keys

#endif  // BRAVE_COMPONENTS_BRAVE_SERVICE_KEYS_SERVICE_KEY_UTILS_H_
