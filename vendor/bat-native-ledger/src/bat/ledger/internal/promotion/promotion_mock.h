/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PROMOTION_PROMOTION_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PROMOTION_PROMOTION_MOCK_H_

#include "bat/ledger/internal/promotion/promotion.h"
#include "bat/ledger/ledger.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ledger {
namespace promotion {

class MockPromotion : public Promotion {
 public:
  explicit MockPromotion(LedgerImpl* ledger);

  ~MockPromotion() override;

  MOCK_METHOD1(TransferTokens,
               void(ledger::PostSuggestionsClaimCallback callback));
};

}  // namespace promotion
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_PROMOTION_PROMOTION_MOCK_H_
