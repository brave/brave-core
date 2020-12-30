/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_MOCK_H_  // NOLINT
#define BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_MOCK_H_  // NOLINT

#include "testing/gmock/include/gmock/gmock.h"
#include "bat/ads/internal/tokens/refill_unblinded_tokens/refill_unblinded_tokens_delegate.h"

namespace ads {

class RefillUnblindedTokensDelegateMock
    : public RefillUnblindedTokensDelegate {
 public:
  RefillUnblindedTokensDelegateMock();

  ~RefillUnblindedTokensDelegateMock() override;

  RefillUnblindedTokensDelegateMock(
      const RefillUnblindedTokensDelegateMock&) = delete;
  RefillUnblindedTokensDelegateMock& operator=(
      const RefillUnblindedTokensDelegateMock&) = delete;

  MOCK_METHOD(void, OnDidRefillUnblindedTokens, ());

  MOCK_METHOD(void, OnFailedToRefillUnblindedTokens, ());

  MOCK_METHOD(void, OnWillRetryRefillingUnblindedTokens, ());
  MOCK_METHOD(void, OnDidRetryRefillingUnblindedTokens, ());
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_DELEGATE_MOCK_H_  // NOLINT
