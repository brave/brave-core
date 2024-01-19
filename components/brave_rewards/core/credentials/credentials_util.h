/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_UTIL_H_

#include <optional>
#include <string>
#include <vector>

#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/credentials/credentials_redeem.h"
#include "brave/components/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/challenge_bypass_ristretto/token.h"

using challenge_bypass_ristretto::BlindedToken;
using challenge_bypass_ristretto::Token;

namespace brave_rewards::internal {
namespace credential {

std::vector<Token> GenerateCreds(const int count);

std::string GetCredsJSON(const std::vector<Token>& creds);

std::vector<BlindedToken> GenerateBlindCreds(const std::vector<Token>& tokens);

std::string GetBlindedCredsJSON(const std::vector<BlindedToken>& blinded);

std::optional<base::Value::List> ParseStringToBaseList(
    const std::string& string_list);

base::expected<std::vector<std::string>, std::string> UnBlindCreds(
    const mojom::CredsBatch& creds);

std::vector<std::string> UnBlindCredsMock(const mojom::CredsBatch& creds);

std::string ConvertRewardTypeToString(const mojom::RewardsType type);

base::Value::List GenerateCredentials(
    const std::vector<mojom::UnblindedToken>& token_list,
    const std::string& body);

std::optional<base::Value::Dict> GenerateSuggestion(
    const std::string& token_value,
    const std::string& public_key,
    const std::string& suggestion_encoded);

base::Value::Dict GenerateSuggestionMock(const std::string& token_value,
                                         const std::string& public_key,
                                         const std::string& suggestion_encoded);

}  // namespace credential
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_CREDENTIALS_CREDENTIALS_UTIL_H_
