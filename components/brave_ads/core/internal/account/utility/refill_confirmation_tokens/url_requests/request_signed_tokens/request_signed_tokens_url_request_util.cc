/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/request_signed_tokens/request_signed_tokens_url_request_util.h"

namespace brave_ads {

namespace {

constexpr char kIsEligibleKey[] = "isEligible";
constexpr char kNonceKey[] = "nonce";

}  // namespace

std::optional<bool> ParseIsEligible(const base::Value::Dict& dict) {
  return dict.FindBool(kIsEligibleKey);
}

std::optional<std::string> ParseNonce(const base::Value::Dict& dict) {
  const std::string* const nonce = dict.FindString(kNonceKey);
  if (!nonce || nonce->empty()) {
    return std::nullopt;
  }

  return *nonce;
}

}  // namespace brave_ads
