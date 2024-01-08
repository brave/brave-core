// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONSTANTS_BRAVE_SERVICES_KEY_V2_H_
#define BRAVE_COMPONENTS_CONSTANTS_BRAVE_SERVICES_KEY_V2_H_

#include <optional>
#include <string>
#include <utility>

namespace constants {

enum class Service {
  kAIChat,
};

std::optional<std::pair<std::string, std::string>> GetBraveServicesV2Headers(
    const std::string payload,
    Service service);

}  // namespace constants

#endif  // BRAVE_COMPONENTS_CONSTANTS_BRAVE_SERVICES_KEY_V2_H_
