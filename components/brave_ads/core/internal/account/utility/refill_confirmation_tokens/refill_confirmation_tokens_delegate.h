/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_CONFIRMATION_TOKENS_REFILL_CONFIRMATION_TOKENS_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_CONFIRMATION_TOKENS_REFILL_CONFIRMATION_TOKENS_DELEGATE_H_

#include <string>

#include "base/time/time.h"

namespace brave_ads {

class RefillConfirmationTokensDelegate {
 public:
  // Invoked to tell the delegate we will refill confirmation tokens.
  virtual void OnWillRefillConfirmationTokens() {}

  // Invoked to tell the delegate we successfully refilled the confirmation
  // tokens.
  virtual void OnDidRefillConfirmationTokens() {}

  // Invoked to tell the delegate we failed to refill the confirmation tokens.
  virtual void OnFailedToRefillConfirmationTokens() {}

  // Invoked to tell the delegate that we will retry refilling the confirmation
  // tokens at `retry_at`.
  virtual void OnWillRetryRefillingConfirmationTokens(base::Time retry_at) {}

  // Invoked to tell the delegate that we retried refilling the confirmation
  // tokens.
  virtual void OnDidRetryRefillingConfirmationTokens() {}

  // Invoked to tell the delegate that the user must solve a scheduled captcha
  // with the given `captcha_id` before we can refill the confirmation tokens.
  virtual void OnCaptchaRequiredToRefillConfirmationTokens(
      const std::string& captcha_id) {}

 protected:
  virtual ~RefillConfirmationTokensDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_CONFIRMATION_TOKENS_REFILL_CONFIRMATION_TOKENS_DELEGATE_H_
