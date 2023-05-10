/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_GEMINI_GEMINI_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_GEMINI_GEMINI_UTIL_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/common/mojom/ledger.mojom.h"
#include "brave/components/brave_rewards/common/mojom/ledger_types.mojom.h"

namespace brave_rewards::internal::gemini {

inline constexpr char kGeminiRecipientIDLabel[] = "Brave Browser";

std::string GetClientId();

std::string GetClientSecret();

std::string GetFeeAddress();

mojom::ExternalWalletPtr GenerateLinks(mojom::ExternalWalletPtr wallet);

}  // namespace brave_rewards::internal::gemini

namespace brave_rewards::internal::endpoint::gemini {

std::vector<std::string> RequestAuthorization(const std::string& token = "");

std::string GetApiServerUrl(const std::string& path);
std::string GetOauthServerUrl(const std::string& path);

mojom::Result CheckStatusCode(int status_code);

}  // namespace brave_rewards::internal::endpoint::gemini

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_GEMINI_GEMINI_UTIL_H_
