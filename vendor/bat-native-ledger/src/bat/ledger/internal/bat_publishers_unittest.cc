/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/bat_publishers.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatPublishersTest.*

namespace braveledger_bat_publishers {

class BatPublishersTest : public testing::Test {
};

TEST_F(BatPublishersTest, calcScoreConsts) {
  braveledger_bat_publishers::BatPublishers* publishers =
      new braveledger_bat_publishers::BatPublishers(nullptr);

  /*
   * Test 5 seconds
   */
  publishers->calcScoreConsts(5);

  EXPECT_EQ(publishers->a_, 14500);
  EXPECT_EQ(publishers->a2_, 29000);
  EXPECT_EQ(publishers->a4_, 58000);
  EXPECT_EQ(publishers->b_, -14000);
  EXPECT_EQ(publishers->b2_, 196000000);

  /*
   * Test 8 seconds
   */
  publishers->calcScoreConsts(8);

  EXPECT_EQ(publishers->a_, 14200);
  EXPECT_EQ(publishers->a2_, 28400);
  EXPECT_EQ(publishers->a4_, 56800);
  EXPECT_EQ(publishers->b_, -13400);
  EXPECT_EQ(publishers->b2_, 179560000);

  /*
   * Test 1min (60 seconds)
   */
  publishers->calcScoreConsts(60);

  EXPECT_EQ(publishers->a_, 9000);
  EXPECT_EQ(publishers->a2_, 18000);
  EXPECT_EQ(publishers->a4_, 36000);
  EXPECT_EQ(publishers->b_, -3000);
  EXPECT_EQ(publishers->b2_, 9000000);
}

TEST_F(BatPublishersTest, concaveScore) {
  braveledger_bat_publishers::BatPublishers* publishers =
      new braveledger_bat_publishers::BatPublishers(nullptr);

  /*
   * min duration: 5 seconds
   * duration: 5, 15, 60, 1000, 10000, 150000, 500000
   */
  publishers->calcScoreConsts(5);
  EXPECT_NEAR(publishers->concaveScore(5), 1, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(15), 1.06285, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(60), 1.28703, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(1000), 3.15289, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(10000), 8.80133, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(150000), 32.6498, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(500000), 59.2068, 0.001f);

  /*
   * min duration: 8 seconds
   * duration: 5, 15, 60, 1000, 10000, 150000, 500000
   */
  publishers->calcScoreConsts(8);
  EXPECT_NEAR(publishers->concaveScore(5), 0.979606, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(15), 1.04477, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(60), 1.27505, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(1000), 3.16717, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(10000), 8.8769, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(150000), 32.9766, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(500000), 59.8128, 0.001f);

  /*
   * min duration: 60 seconds
   * duration: 5, 15, 60, 1000, 10000, 150000, 500000
   */
  publishers->calcScoreConsts(60);
  EXPECT_NEAR(publishers->concaveScore(5), 0.455342, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(15), 0.607625, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(60), 1, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(1000), 3.50416, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(10000), 10.7089, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(150000), 40.9918, 0.001f);
  EXPECT_NEAR(publishers->concaveScore(500000), 74.7025, 0.001f);
}

}  // namespace braveledger_bat_publishers
