/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_CONFIRMATION_TOKENS_REFILL_CONFIRMATION_TOKENS_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_CONFIRMATION_TOKENS_REFILL_CONFIRMATION_TOKENS_DELEGATE_MOCK_H_

#include <string>

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class RefillConfirmationTokensDelegateMock
    : public RefillConfirmationTokensDelegate {
 public:
  RefillConfirmationTokensDelegateMock();

  RefillConfirmationTokensDelegateMock(
      const RefillConfirmationTokensDelegateMock&) = delete;
  RefillConfirmationTokensDelegateMock& operator=(
      const RefillConfirmationTokensDelegateMock&) = delete;

  ~RefillConfirmationTokensDelegateMock() override;

  MOCK_METHOD(void, OnDidRefillConfirmationTokens, ());

  MOCK_METHOD(void, OnFailedToRefillConfirmationTokens, ());

  MOCK_METHOD(void,
              OnWillRetryRefillingConfirmationTokens,
              (const base::Time retry_at));
  MOCK_METHOD(void, OnDidRetryRefillingConfirmationTokens, ());

  MOCK_METHOD(void,
              OnCaptchaRequiredToRefillConfirmationTokens,
              (const std::string& captcha_id));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_CONFIRMATION_TOKENS_REFILL_CONFIRMATION_TOKENS_DELEGATE_MOCK_H_
