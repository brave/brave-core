/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_REDEEM_TOKEN_MOCK_H_
#define BAT_CONFIRMATIONS_INTERNAL_REDEEM_TOKEN_MOCK_H_

#include "bat/confirmations/internal/redeem_unblinded_token_delegate.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace confirmations {

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

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_REDEEM_TOKEN_MOCK_H_
