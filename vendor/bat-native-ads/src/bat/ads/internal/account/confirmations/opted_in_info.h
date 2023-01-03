/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_OPTED_IN_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_OPTED_IN_INFO_H_

#include <string>

#include "absl/types/optional.h"
#include "base/values.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/token.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"

namespace ads {

struct OptedInInfo final {
  OptedInInfo();

  OptedInInfo(const OptedInInfo& other);
  OptedInInfo& operator=(const OptedInInfo& other);

  OptedInInfo(OptedInInfo&& other) noexcept;
  OptedInInfo& operator=(OptedInInfo&& other) noexcept;

  ~OptedInInfo();

  privacy::cbr::Token token;
  privacy::cbr::BlindedToken blinded_token;
  privacy::UnblindedTokenInfo unblinded_token;
  base::Value::Dict user_data;
  absl::optional<std::string> credential_base64url;
};

bool operator==(const OptedInInfo& lhs, const OptedInInfo& rhs);
bool operator!=(const OptedInInfo& lhs, const OptedInInfo& rhs);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_OPTED_IN_INFO_H_
