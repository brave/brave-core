/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_OPTED_IN_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_OPTED_IN_INFO_H_

#include <string>

#include "brave/components/brave_ads/core/internal/account/confirmations/opted_in_user_data_info.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

struct OptedInInfo final {
  OptedInInfo();

  OptedInInfo(const OptedInInfo&);
  OptedInInfo& operator=(const OptedInInfo&);

  OptedInInfo(OptedInInfo&&) noexcept;
  OptedInInfo& operator=(OptedInInfo&&) noexcept;

  ~OptedInInfo();

  privacy::cbr::Token token;
  privacy::cbr::BlindedToken blinded_token;
  privacy::UnblindedTokenInfo unblinded_token;
  OptedInUserDataInfo user_data;
  absl::optional<std::string> credential_base64url;
};

bool operator==(const OptedInInfo&, const OptedInInfo&);
bool operator!=(const OptedInInfo&, const OptedInInfo&);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_OPTED_IN_INFO_H_
