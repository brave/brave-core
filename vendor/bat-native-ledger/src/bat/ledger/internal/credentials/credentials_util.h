/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_UTIL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/values.h"
#include "bat/ledger/internal/credentials/credentials_redeem.h"
#include "bat/ledger/mojom_structs.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

#include "wrapper.hpp"

using challenge_bypass_ristretto::Token;
using challenge_bypass_ristretto::BlindedToken;

namespace ledger {
namespace credential {

std::vector<Token> GenerateCreds(const int count);

std::string GetCredsJSON(const std::vector<Token>& creds);

std::vector<BlindedToken> GenerateBlindCreds(
    const std::vector<Token>& tokens);

std::string GetBlindedCredsJSON(const std::vector<BlindedToken>& blinded);

absl::optional<base::Value::List> ParseStringToBaseList(
    const std::string& string_list);

bool UnBlindCreds(const mojom::CredsBatch& creds,
                  std::vector<std::string>* unblinded_encoded_creds,
                  std::string* error);

bool UnBlindCredsMock(const mojom::CredsBatch& creds,
                      std::vector<std::string>* unblinded_encoded_creds);

std::string ConvertRewardTypeToString(const mojom::RewardsType type);

base::Value::List GenerateCredentials(
    const std::vector<mojom::UnblindedToken>& token_list,
    const std::string& body);

absl::optional<base::Value::Dict> GenerateSuggestion(
    const std::string& token_value,
    const std::string& public_key,
    const std::string& suggestion_encoded);

base::Value::Dict GenerateSuggestionMock(const std::string& token_value,
                                         const std::string& public_key,
                                         const std::string& suggestion_encoded);

}  // namespace credential
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CREDENTIALS_CREDENTIALS_UTIL_H_
