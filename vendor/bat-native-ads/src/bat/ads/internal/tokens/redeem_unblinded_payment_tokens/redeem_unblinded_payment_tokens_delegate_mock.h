/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_MOCK_H_  // NOLINT
#define BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_MOCK_H_  // NOLINT

#include "base/time/time.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "bat/ads/internal/tokens/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate.h"

namespace ads {

class RedeemUnblindedPaymentTokensDelegateMock
    : public RedeemUnblindedPaymentTokensDelegate {
 public:
  RedeemUnblindedPaymentTokensDelegateMock();

  ~RedeemUnblindedPaymentTokensDelegateMock() override;

  RedeemUnblindedPaymentTokensDelegateMock(
      const RedeemUnblindedPaymentTokensDelegateMock&) = delete;
  RedeemUnblindedPaymentTokensDelegateMock& operator=(
      const RedeemUnblindedPaymentTokensDelegateMock&) = delete;

  MOCK_METHOD(void, OnDidRedeemUnblindedPaymentTokens, ());
  MOCK_METHOD(void, OnFailedToRedeemUnblindedPaymentTokens, ());

  MOCK_METHOD(void, OnDidScheduleNextUnblindedPaymentTokensRedemption, (
      const base::Time& time));

  MOCK_METHOD(void, OnWillRetryRedeemingUnblindedPaymentTokens, ());
  MOCK_METHOD(void, OnDidRetryRedeemingUnblindedPaymentTokens, ());
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_DELEGATE_MOCK_H_  // NOLINT
