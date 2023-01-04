/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_MOCK_H_

#include <string>

#include "bat/ads/internal/account/utility/refill_unblinded_tokens/refill_unblinded_tokens_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep

namespace ads {

class RefillUnblindedTokensDelegateMock : public RefillUnblindedTokensDelegate {
 public:
  RefillUnblindedTokensDelegateMock();

  RefillUnblindedTokensDelegateMock(const RefillUnblindedTokensDelegateMock&) =
      delete;
  RefillUnblindedTokensDelegateMock& operator=(
      const RefillUnblindedTokensDelegateMock& other) = delete;

  RefillUnblindedTokensDelegateMock(
      RefillUnblindedTokensDelegateMock&& other) noexcept = delete;
  RefillUnblindedTokensDelegateMock& operator=(
      RefillUnblindedTokensDelegateMock&& other) noexcept = delete;

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

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_MOCK_H_
