/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"

#include <tuple>

namespace brave_ads::privacy {

bool UnblindedTokenInfo::operator==(const UnblindedTokenInfo& other) const {
  const auto tie = [](const UnblindedTokenInfo& unblinded_token) {
    return std::tie(unblinded_token.public_key, unblinded_token.value,
                    unblinded_token.signature);
  };

  return tie(*this) == tie(other);
}

bool UnblindedTokenInfo::operator!=(const UnblindedTokenInfo& other) const {
  return !(*this == other);
}

}  // namespace brave_ads::privacy
