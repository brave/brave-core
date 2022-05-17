/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKEN_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKEN_INFO_H_

#include "bat/ads/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"

namespace ads {
namespace privacy {

struct UnblindedTokenInfo final {
  UnblindedTokenInfo();
  UnblindedTokenInfo(const UnblindedTokenInfo& info);
  ~UnblindedTokenInfo();

  bool operator==(const UnblindedTokenInfo& rhs) const;
  bool operator!=(const UnblindedTokenInfo& rhs) const;

  bool is_valid() const;

  cbr::UnblindedToken value;
  cbr::PublicKey public_key;
};

}  // namespace privacy
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PRIVACY_TOKENS_UNBLINDED_TOKENS_UNBLINDED_TOKEN_INFO_H_
