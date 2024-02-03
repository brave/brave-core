/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/issuers/confirmations_issuer_util.h"

#include <cstddef>
#include <optional>

#include "brave/components/brave_ads/core/internal/account/issuers/issuer_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuer_types.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_feature.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"

namespace brave_ads {

bool IsConfirmationsIssuerValid(const IssuersInfo& issuers) {
  const std::optional<IssuerInfo> confirmations_issuer =
      GetIssuerForType(issuers, IssuerType::kConfirmations);
  if (!confirmations_issuer) {
    return false;
  }

  return confirmations_issuer->public_keys.size() <=
         static_cast<size_t>(kMaximumIssuerPublicKeys.Get());
}

}  // namespace brave_ads
