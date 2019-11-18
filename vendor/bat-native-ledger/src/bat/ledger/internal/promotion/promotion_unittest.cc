/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "bat/ledger/internal/promotion/promotion_util.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PromotionTest.*

namespace braveledger_promotion {

class PromotionTest : public testing::Test {
};

TEST_F(PromotionTest, VerifyPublicKey) {
  auto promotion = ledger::Promotion::New();
  auto credentials = ledger::PromotionCreds::New();

  // null pointer
  bool result = braveledger_promotion::VerifyPublicKey(nullptr);
  EXPECT_EQ(result, false);

  // credentials are not set
  result = braveledger_promotion::VerifyPublicKey(promotion->Clone());
  EXPECT_EQ(result, false);

  // keys are not formatted correctly
  promotion->credentials = std::move(credentials);
  promotion->public_keys = "fdsfsdds";
  result = braveledger_promotion::VerifyPublicKey(promotion->Clone());
  EXPECT_EQ(result, false);

  // keys doesn't match
  promotion->public_keys = "[\"orBZ6dkSFLwBtQgI+5qXFb0dzDSm4uf+Km6AhytgUT8=\"]";
  promotion->credentials->public_key = "dfsdfsdf";
  result = braveledger_promotion::VerifyPublicKey(promotion->Clone());
  EXPECT_EQ(result, false);

  // keys match
  promotion->public_keys = "[\"orBZ6dkSFLwBtQgI+5qXFb0dzDSm4uf+Km6AhytgUT8=\"]";
  promotion->credentials->public_key =
      "orBZ6dkSFLwBtQgI+5qXFb0dzDSm4uf+Km6AhytgUT8=";
  result = braveledger_promotion::VerifyPublicKey(promotion->Clone());
  EXPECT_EQ(result, true);
}

}  // namespace braveledger_promotion
