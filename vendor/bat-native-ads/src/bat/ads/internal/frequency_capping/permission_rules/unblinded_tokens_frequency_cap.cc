/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/unblinded_tokens_frequency_cap.h"

#include "bat/ads/internal/account/wallet.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/internal/tokens/refill_unblinded_tokens/refill_unblinded_tokens.h"

namespace ads {

namespace {
const int kUnblindedTokensMinimumThreshold = 10;
}  // namespace

UnblindedTokensFrequencyCap::UnblindedTokensFrequencyCap(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

UnblindedTokensFrequencyCap::~UnblindedTokensFrequencyCap() = default;

bool UnblindedTokensFrequencyCap::ShouldAllow() {
  if (!DoesRespectCap()) {
    const WalletInfo wallet = ads_->get_wallet()->Get();
    ads_->get_refill_unblinded_tokens()->MaybeRefill(wallet);

    last_message_ = "You do not have enough unblinded tokens";
    return false;
  }

  return true;
}

std::string UnblindedTokensFrequencyCap::get_last_message() const {
  return last_message_;
}

bool UnblindedTokensFrequencyCap::DoesRespectCap() {
  const int count = ads_->get_confirmations()->get_unblinded_tokens()->Count();
  if (count < kUnblindedTokensMinimumThreshold) {
    return false;
  }

  return true;
}

}  // namespace ads
