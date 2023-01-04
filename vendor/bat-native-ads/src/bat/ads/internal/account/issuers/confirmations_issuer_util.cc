/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/issuers/confirmations_issuer_util.h"

#include "absl/types/optional.h"
#include "bat/ads/internal/account/issuers/issuer_constants.h"
#include "bat/ads/internal/account/issuers/issuer_info.h"
#include "bat/ads/internal/account/issuers/issuer_types.h"
#include "bat/ads/internal/account/issuers/issuers_info.h"
#include "bat/ads/internal/account/issuers/issuers_util.h"

namespace ads {

bool IsConfirmationsIssuerValid(const IssuersInfo& issuers) {
  const absl::optional<IssuerInfo> confirmations_issuer =
      GetIssuerForType(issuers, IssuerType::kConfirmations);
  if (!confirmations_issuer) {
    return false;
  }

  return confirmations_issuer->public_keys.size() <= kMaximumIssuerPublicKeys;
}

}  // namespace ads
