/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_builder_util.h"

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_ads/core/internal/account/utility/tokens_constants.h"

namespace brave_ads {

std::string BuildGetSignedTokensUrlPath(const std::string& payment_id,
                                        const std::string& nonce) {
  CHECK(!payment_id.empty());
  CHECK(!nonce.empty());

  return absl::StrFormat("/v%d/confirmation/token/%s?nonce=%s",
                         kTokensServerVersion, payment_id, nonce);
}

}  // namespace brave_ads
