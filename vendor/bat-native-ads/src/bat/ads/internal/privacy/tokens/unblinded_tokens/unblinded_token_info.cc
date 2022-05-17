/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"

namespace ads {
namespace privacy {

UnblindedTokenInfo::UnblindedTokenInfo() = default;

UnblindedTokenInfo::UnblindedTokenInfo(
    const UnblindedTokenInfo& unblinded_token) = default;

UnblindedTokenInfo::~UnblindedTokenInfo() = default;

bool UnblindedTokenInfo::operator==(const UnblindedTokenInfo& rhs) const {
  return public_key == rhs.public_key && value == rhs.value;
}

bool UnblindedTokenInfo::operator!=(const UnblindedTokenInfo& rhs) const {
  return !(*this == rhs);
}

bool UnblindedTokenInfo::is_valid() const {
  return public_key.has_value() && value.has_value();
}

}  // namespace privacy
}  // namespace ads
