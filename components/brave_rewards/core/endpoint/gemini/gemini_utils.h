/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_GEMINI_GEMINI_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_GEMINI_GEMINI_UTILS_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/ledger_callbacks.h"

namespace brave_rewards::internal {
namespace endpoints {
inline const char kGeminiRecipientIDLabel[] = "Brave Browser";
}
namespace endpoint::gemini {

std::string GetClientId();

std::string GetClientSecret();

std::vector<std::string> RequestAuthorization(const std::string& token = "");

std::string GetApiServerUrl(const std::string& path);
std::string GetOauthServerUrl(const std::string& path);

mojom::Result CheckStatusCode(const int status_code);

}  // namespace endpoint::gemini
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_GEMINI_GEMINI_UTILS_H_
