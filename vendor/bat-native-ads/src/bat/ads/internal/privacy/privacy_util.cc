/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/privacy_util.h"

#include "base/check.h"
#include "wrapper.hpp"

namespace ads {

using challenge_bypass_ristretto::BlindedToken;
using challenge_bypass_ristretto::Token;

namespace privacy {
namespace cbr {

BlindedTokenList BlindTokens(const TokenList& tokens) {
  DCHECK(!tokens.empty());

  BlindedTokenList blinded_tokens;
  for (Token token : tokens) {
    const BlindedToken& blinded_token = token.blind();
    blinded_tokens.push_back(blinded_token);
  }

  return blinded_tokens;
}

}  // namespace cbr
}  // namespace privacy
}  // namespace ads
