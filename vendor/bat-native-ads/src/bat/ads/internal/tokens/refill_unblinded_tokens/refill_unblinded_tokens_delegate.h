/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_H_

#include <string>

namespace ads {

class RefillUnblindedTokensDelegate {
 public:
  virtual ~RefillUnblindedTokensDelegate() = default;

  // Invoked to tell the delegate unblinded tokens were refilled
  virtual void OnDidRefillUnblindedTokens() {}

  // Invoked to tell the delegate unblinded tokens failed to refill
  virtual void OnFailedToRefillUnblindedTokens() {}

  // Invoked to tell the delegate that we will retry refilling unblinded tokens
  virtual void OnWillRetryRefillingUnblindedTokens() {}

  // Invoked to tell the delegate that we did retry refilling unblinded tokens
  virtual void OnDidRetryRefillingUnblindedTokens() {}

  // Invoked to tell the delegate that the user must solve a scheduled
  // captcha before refilling unblinded tokens
  virtual void OnCaptchaRequiredToRefillUnblindedTokens(
      const std::string& captcha_id) {}
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_H_
