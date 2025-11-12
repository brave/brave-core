/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/payload/reward_confirmation_payload_util.h"

#include <optional>
#include <string>
#include <string_view>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_info.h"

namespace brave_ads {

namespace {

constexpr std::string_view kBlindedTokensKey = "blindedPaymentTokens";
constexpr std::string_view kPublicKeyKey = "publicKey";

}  // namespace

base::Value::Dict BuildRewardConfirmationPayload(const RewardInfo& reward) {
  std::optional<std::string> blinded_token_base64 =
      reward.blinded_token.EncodeBase64();
  CHECK(blinded_token_base64);

  std::optional<std::string> public_key_base64 =
      reward.public_key.EncodeBase64();
  CHECK(public_key_base64);

  return base::Value::Dict()
      .Set(kBlindedTokensKey, base::Value::List().Append(*blinded_token_base64))
      .Set(kPublicKeyKey, *public_key_base64);
}

}  // namespace brave_ads
