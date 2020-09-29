/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/unblinded_tokens_frequency_cap.h"

#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/server/refill_unblinded_tokens/refill_unblinded_tokens.h"

namespace ads {

namespace {
const int kUnblindedTokenCountThreshold = 10;
}  // namespace

UnblindedTokensFrequencyCap::UnblindedTokensFrequencyCap(
    const AdsImpl* const ads)
    : ads_(ads) {
  DCHECK(ads_);
}

UnblindedTokensFrequencyCap::~UnblindedTokensFrequencyCap() = default;

bool UnblindedTokensFrequencyCap::IsAllowed() {
  if (!DoesRespectCap()) {
    last_message_ = "You do not have enough unblinded tokens";

    ads_->get_refill_unblinded_tokens()->MaybeRefill();

    return false;
  }

  return true;
}

std::string UnblindedTokensFrequencyCap::get_last_message() const {
  return last_message_;
}

bool UnblindedTokensFrequencyCap::DoesRespectCap() const {
  const int count = ads_->get_confirmations()->get_unblinded_tokens()->Count();

  if (count < kUnblindedTokenCountThreshold) {
    return false;
  }

  return true;
}

}  // namespace ads
