/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"

namespace ads::privacy {

bool UnblindedTokenInfo::operator==(const UnblindedTokenInfo& other) const {
  return public_key == other.public_key && value == other.value &&
         signature == other.signature;
}

bool UnblindedTokenInfo::operator!=(const UnblindedTokenInfo& other) const {
  return !(*this == other);
}

}  // namespace ads::privacy
