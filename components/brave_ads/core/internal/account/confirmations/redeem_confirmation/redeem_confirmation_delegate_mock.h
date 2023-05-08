/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_CONFIRMATION_REDEEM_CONFIRMATION_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_CONFIRMATION_REDEEM_CONFIRMATION_DELEGATE_MOCK_H_

#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_confirmation/redeem_confirmation_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class RedeemConfirmationDelegateMock : public RedeemConfirmationDelegate {
 public:
  RedeemConfirmationDelegateMock();

  RedeemConfirmationDelegateMock(const RedeemConfirmationDelegateMock&) =
      delete;
  RedeemConfirmationDelegateMock& operator=(
      const RedeemConfirmationDelegateMock&) = delete;

  RedeemConfirmationDelegateMock(RedeemConfirmationDelegateMock&&) noexcept =
      delete;
  RedeemConfirmationDelegateMock& operator=(
      RedeemConfirmationDelegateMock&&) noexcept = delete;

  ~RedeemConfirmationDelegateMock() override;

  MOCK_METHOD(
      void,
      OnDidRedeemOptedInConfirmation,
      (const ConfirmationInfo& confirmation,
       const privacy::UnblindedPaymentTokenInfo& unblinded_payment_token));

  MOCK_METHOD(void,
              OnDidRedeemOptedOutConfirmation,
              (const ConfirmationInfo& confirmation));

  MOCK_METHOD(void,
              OnFailedToRedeemConfirmation,
              (const ConfirmationInfo& confirmation,
               const bool should_retry,
               const bool should_backoff));
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_CONFIRMATION_REDEEM_CONFIRMATION_DELEGATE_MOCK_H_
