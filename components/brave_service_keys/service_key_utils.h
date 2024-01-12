// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SERVICE_KEYS_SERVICE_KEY_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_SERVICE_KEYS_SERVICE_KEY_UTILS_H_

#include <optional>
#include <string>
#include <utility>

#include "base/containers/span.h"
#include "base/strings/strcat.h"

namespace brave_service_keys {

std::pair<std::string, std::string> GetDigestHeader(const std::string& payload);

std::optional<std::pair<std::string, std::string>> GetAuthorizationHeader(
    const std::string& service_key,
    base::span<const std::pair<std::string, std::string>> headers);

}  // namespace brave_service_keys

#endif  // BRAVE_COMPONENTS_BRAVE_SERVICE_KEYS_SERVICE_KEY_UTILS_H_
