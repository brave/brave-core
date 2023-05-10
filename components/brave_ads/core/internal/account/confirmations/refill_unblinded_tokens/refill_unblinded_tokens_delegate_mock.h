/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_MOCK_H_

#include <string>

#include "brave/components/brave_ads/core/internal/account/confirmations/refill_unblinded_tokens/refill_unblinded_tokens_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class RefillUnblindedTokensDelegateMock : public RefillUnblindedTokensDelegate {
 public:
  RefillUnblindedTokensDelegateMock();

  RefillUnblindedTokensDelegateMock(const RefillUnblindedTokensDelegateMock&) =
      delete;
  RefillUnblindedTokensDelegateMock& operator=(
      const RefillUnblindedTokensDelegateMock&) = delete;

  RefillUnblindedTokensDelegateMock(
      RefillUnblindedTokensDelegateMock&&) noexcept = delete;
  RefillUnblindedTokensDelegateMock& operator=(
      RefillUnblindedTokensDelegateMock&&) noexcept = delete;

  ~RefillUnblindedTokensDelegateMock() override;

  MOCK_METHOD(void, OnDidRefillUnblindedTokens, ());

  MOCK_METHOD(void, OnFailedToRefillUnblindedTokens, ());

  MOCK_METHOD(void,
              OnWillRetryRefillingUnblindedTokens,
              (const base::Time retry_at));
  MOCK_METHOD(void, OnDidRetryRefillingUnblindedTokens, ());

  MOCK_METHOD(void,
              OnCaptchaRequiredToRefillUnblindedTokens,
              (const std::string& captcha_id));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_MOCK_H_
