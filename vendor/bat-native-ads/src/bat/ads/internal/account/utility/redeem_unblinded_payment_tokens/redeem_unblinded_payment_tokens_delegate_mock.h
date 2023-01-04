/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_MOCK_H_

#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep

namespace base {
class Time;
}  // namespace base

namespace ads {

class RedeemUnblindedPaymentTokensDelegateMock
    : public RedeemUnblindedPaymentTokensDelegate {
 public:
  RedeemUnblindedPaymentTokensDelegateMock();

  RedeemUnblindedPaymentTokensDelegateMock(
      const RedeemUnblindedPaymentTokensDelegateMock& other) = delete;
  RedeemUnblindedPaymentTokensDelegateMock& operator=(
      const RedeemUnblindedPaymentTokensDelegateMock& other) = delete;

  RedeemUnblindedPaymentTokensDelegateMock(
      RedeemUnblindedPaymentTokensDelegateMock&& other) noexcept = delete;
  RedeemUnblindedPaymentTokensDelegateMock& operator=(
      RedeemUnblindedPaymentTokensDelegateMock&& other) noexcept = delete;

  ~RedeemUnblindedPaymentTokensDelegateMock() override;

  MOCK_METHOD(
      void,
      OnDidRedeemUnblindedPaymentTokens,
      (const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens));
  MOCK_METHOD(void, OnFailedToRedeemUnblindedPaymentTokens, ());

  MOCK_METHOD(void,
              OnDidScheduleNextUnblindedPaymentTokensRedemption,
              (const base::Time redeem_at));

  MOCK_METHOD(void,
              OnWillRetryRedeemingUnblindedPaymentTokens,
              (const base::Time retry_at));
  MOCK_METHOD(void, OnDidRetryRedeemingUnblindedPaymentTokens, ());
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_MOCK_H_
