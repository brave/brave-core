/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_H_

#include <string>

#include "base/time/time.h"

namespace brave_ads {

class RefillUnblindedTokensDelegate {
 public:
  // Invoked to tell the delegate we successfully refilled the unblinded tokens.
  virtual void OnDidRefillUnblindedTokens() {}

  // Invoked to tell the delegate we failed to refill the unblinded tokens.
  virtual void OnFailedToRefillUnblindedTokens() {}

  // Invoked to tell the delegate that we will retry refilling the unblinded
  // tokens at |retry_at|.
  virtual void OnWillRetryRefillingUnblindedTokens(const base::Time retry_at) {}

  // Invoked to tell the delegate that we retried refilling the unblinded
  // tokens.
  virtual void OnDidRetryRefillingUnblindedTokens() {}

  // Invoked to tell the delegate that the user must solve a scheduled captcha
  // with the given |captcha_id| before we can refill the unblinded tokens.
  virtual void OnCaptchaRequiredToRefillUnblindedTokens(
      const std::string& captcha_id) {}

 protected:
  virtual ~RefillUnblindedTokensDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_H_
