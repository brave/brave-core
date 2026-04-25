/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/non_reward/url_request_builders/create_non_reward_confirmation_url_request_builder_util.h"

#include "base/check.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_ads {

namespace {
constexpr int kConfirmationServerVersion = 4;
}  // namespace

std::string BuildCreateNonRewardConfirmationUrlPath(
    const std::string& transaction_id) {
  CHECK(!transaction_id.empty());

  return absl::StrFormat("/v%d/confirmation/%s", kConfirmationServerVersion,
                         transaction_id);
}

}  // namespace brave_ads
