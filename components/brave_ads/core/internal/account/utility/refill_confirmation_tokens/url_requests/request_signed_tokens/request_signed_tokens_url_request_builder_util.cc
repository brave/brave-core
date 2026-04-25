/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/request_signed_tokens/request_signed_tokens_url_request_builder_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/utility/tokens_constants.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_ads {

std::string BuildRequestSignedTokensUrlPath(const std::string& payment_id) {
  CHECK(!payment_id.empty());

  return absl::StrFormat("/v%d/confirmation/token/%s", kTokensServerVersion,
                         payment_id);
}

}  // namespace brave_ads
