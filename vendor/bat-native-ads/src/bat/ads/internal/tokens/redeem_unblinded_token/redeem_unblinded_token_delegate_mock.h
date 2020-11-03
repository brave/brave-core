/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_DELEGATE_MOCK_H_  // NOLINT
#define BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_DELEGATE_MOCK_H_  // NOLINT

#include "testing/gmock/include/gmock/gmock.h"
#include "bat/ads/internal/tokens/redeem_unblinded_token/redeem_unblinded_token_delegate.h"

namespace ads {

class RedeemUnblindedTokenDelegateMock : public RedeemUnblindedTokenDelegate {
 public:
  RedeemUnblindedTokenDelegateMock();

  ~RedeemUnblindedTokenDelegateMock() override;

  RedeemUnblindedTokenDelegateMock(
      const RedeemUnblindedTokenDelegateMock&) = delete;
  RedeemUnblindedTokenDelegateMock& operator=(
      const RedeemUnblindedTokenDelegateMock&) = delete;

  MOCK_METHOD(void, OnDidRedeemUnblindedToken, (
      const ConfirmationInfo& confirmation));

  MOCK_METHOD(void, OnFailedToRedeemUnblindedToken, (
      const ConfirmationInfo& confirmation));
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_DELEGATE_MOCK_H_  // NOLINT
