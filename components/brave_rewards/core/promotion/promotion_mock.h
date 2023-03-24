/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_MOCK_H_

#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/promotion/promotion.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_rewards::core {
namespace promotion {

class MockPromotion : public Promotion {
 public:
  explicit MockPromotion(LedgerImpl* ledger);

  ~MockPromotion() override;

  MOCK_METHOD1(TransferTokens, void(PostSuggestionsClaimCallback callback));
};

}  // namespace promotion
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PROMOTION_PROMOTION_MOCK_H_
