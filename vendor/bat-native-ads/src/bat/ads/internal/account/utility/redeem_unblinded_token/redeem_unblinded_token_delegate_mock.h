/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_DELEGATE_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_DELEGATE_MOCK_H_

#include "bat/ads/internal/account/utility/redeem_unblinded_token/redeem_unblinded_token_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep

namespace ads {

class RedeemUnblindedTokenDelegateMock : public RedeemUnblindedTokenDelegate {
 public:
  RedeemUnblindedTokenDelegateMock();

  RedeemUnblindedTokenDelegateMock(const RedeemUnblindedTokenDelegateMock&) =
      delete;
  RedeemUnblindedTokenDelegateMock& operator=(
      const RedeemUnblindedTokenDelegateMock& other) = delete;

  RedeemUnblindedTokenDelegateMock(
      RedeemUnblindedTokenDelegateMock&& other) noexcept = delete;
  RedeemUnblindedTokenDelegateMock& operator=(
      RedeemUnblindedTokenDelegateMock&& other) noexcept = delete;

  ~RedeemUnblindedTokenDelegateMock() override;

  MOCK_METHOD(void,
              OnDidSendConfirmation,
              (const ConfirmationInfo& confirmation));

  MOCK_METHOD(void,
              OnFailedToSendConfirmation,
              (const ConfirmationInfo& confirmation, const bool should_retry));

  MOCK_METHOD(
      void,
      OnDidRedeemUnblindedToken,
      (const ConfirmationInfo& confirmation,
       const privacy::UnblindedPaymentTokenInfo& unblinded_payment_token));

  MOCK_METHOD(void,
              OnFailedToRedeemUnblindedToken,
              (const ConfirmationInfo& confirmation,
               const bool should_retry,
               const bool should_backoff));
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_DELEGATE_MOCK_H_
