/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_REDEEM_TOKEN_MOCK_H_
#define BAT_CONFIRMATIONS_INTERNAL_REDEEM_TOKEN_MOCK_H_

#include "bat/confirmations/internal/redeem_token.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace confirmations {

class ConfirmationsImpl;
class UnblindedTokens;
struct ConfirmationInfo;

class RedeemTokenMock : public RedeemToken {
 public:
  RedeemTokenMock(
      ConfirmationsImpl* confirmations,
      UnblindedTokens* unblinded_tokens,
      UnblindedTokens* unblinded_payment_tokens);

  virtual ~RedeemTokenMock();

  MOCK_METHOD(void, OnRedeem, (
      const Result result,
      const ConfirmationInfo& confirmation,
      const bool should_retry),
      (override));
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_REDEEM_TOKEN_MOCK_H_
