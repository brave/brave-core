/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_REDEEM_PAYMENT_TOKENS_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_REDEEM_PAYMENT_TOKENS_DELEGATE_MOCK_H_

#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class RedeemPaymentTokensDelegateMock : public RedeemPaymentTokensDelegate {
 public:
  RedeemPaymentTokensDelegateMock();

  RedeemPaymentTokensDelegateMock(const RedeemPaymentTokensDelegateMock&) =
      delete;
  RedeemPaymentTokensDelegateMock& operator=(
      const RedeemPaymentTokensDelegateMock&) = delete;

  ~RedeemPaymentTokensDelegateMock() override;

  MOCK_METHOD(void,
              OnDidRedeemPaymentTokens,
              (const PaymentTokenList& payment_tokens));
  MOCK_METHOD(void, OnFailedToRedeemPaymentTokens, ());

  MOCK_METHOD(void,
              OnDidScheduleNextPaymentTokenRedemption,
              (const base::Time redeem_at));

  MOCK_METHOD(void,
              OnWillRetryRedeemingPaymentTokens,
              (const base::Time retry_at));
  MOCK_METHOD(void, OnDidRetryRedeemingPaymentTokens, ());
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_REDEEM_PAYMENT_TOKENS_DELEGATE_MOCK_H_
