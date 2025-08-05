/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/reward/url_request_builders/create_reward_confirmation_url_request_builder_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/utility/tokens_constants.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_ads {

std::string BuildCreateRewardConfirmationUrlPath(
    const std::string& transaction_id,
    const std::string& credential_base64url) {
  CHECK(!transaction_id.empty());
  CHECK(!credential_base64url.empty());

  return absl::StrFormat("/v%d/confirmation/%s/%s", kTokensServerVersion,
                         transaction_id, credential_base64url);
}

}  // namespace brave_ads
