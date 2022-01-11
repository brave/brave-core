/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/issuers_frequency_cap.h"

#include "bat/ads/internal/account/account_util.h"
#include "bat/ads/internal/tokens/issuers/issuer_types.h"
#include "bat/ads/internal/tokens/issuers/issuers_util.h"

namespace ads {

IssuersFrequencyCap::IssuersFrequencyCap() = default;

IssuersFrequencyCap::~IssuersFrequencyCap() = default;

bool IssuersFrequencyCap::ShouldAllow() {
  return DoesRespectCap();
}

std::string IssuersFrequencyCap::GetLastMessage() const {
  return last_message_;
}

bool IssuersFrequencyCap::DoesRespectCap() {
  if (!ShouldRewardUser()) {
    return true;
  }

  if (!IssuerExistsForType(IssuerType::kConfirmations) ||
      !IssuerExistsForType(IssuerType::kPayments)) {
    last_message_ = "Missing issuers";
    return false;
  }

  return true;
}

}  // namespace ads
