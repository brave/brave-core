/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/privacy_util.h"

#include "base/check.h"

namespace ads {
namespace privacy {

std::vector<BlindedToken> BlindTokens(const std::vector<Token>& tokens) {
  DCHECK(!tokens.empty());

  std::vector<BlindedToken> blinded_tokens;
  for (unsigned int i = 0; i < tokens.size(); i++) {
    Token token = tokens.at(i);
    const BlindedToken blinded_token = token.blind();

    blinded_tokens.push_back(blinded_token);
  }

  return blinded_tokens;
}

}  // namespace privacy
}  // namespace ads
