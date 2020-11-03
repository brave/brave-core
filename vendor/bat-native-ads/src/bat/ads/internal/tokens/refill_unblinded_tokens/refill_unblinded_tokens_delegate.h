/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_H_  // NOLINT
#define BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_H_  // NOLINT

namespace ads {

class RefillUnblindedTokensDelegate {
 public:
  RefillUnblindedTokensDelegate() = default;

  virtual ~RefillUnblindedTokensDelegate() = default;

  virtual void OnDidRefillUnblindedTokens() = 0;
  virtual void OnFailedToRefillUnblindedTokens() = 0;
  virtual void OnDidRetryRefillingUnblindedTokens() = 0;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_H_  // NOLINT
