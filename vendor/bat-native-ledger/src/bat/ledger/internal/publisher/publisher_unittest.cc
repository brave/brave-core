/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/publisher/publisher.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PublisherTest.*

namespace braveledger_publisher {

class PublisherTest : public testing::Test {
 protected:
  void CreatePublisherInfoList(
      ledger::PublisherInfoList* list) {
    double prev_score;
    for (int ix = 0; ix < 50; ix++) {
      ledger::PublisherInfoPtr info = ledger::PublisherInfo::New();
      info->id = "example" + std::to_string(ix) + ".com";
      info->duration = 50;
      if (ix == 0) {
        info->score = 24;
      } else {
        info->score = prev_score / 2;
      }
      prev_score = info->score;
      info->reconcile_stamp = 0;
      info->visits = 5;
      list->push_back(std::move(info));
    }
  }
};

TEST_F(PublisherTest, calcScoreConsts) {
  braveledger_publisher::Publisher* publishers =
      new braveledger_publisher::Publisher(nullptr);

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

TEST_F(PublisherTest, concaveScore) {
  braveledger_publisher::Publisher* publishers =
      new braveledger_publisher::Publisher(nullptr);

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

TEST_F(PublisherTest, synopsisNormalizerInternal) {
  std::unique_ptr<braveledger_publisher::Publisher> bat_publishers =
      std::make_unique<braveledger_publisher::Publisher>(nullptr);
  // create test PublisherInfo list
  ledger::PublisherInfoList new_list;
  ledger::PublisherInfoList list;
  CreatePublisherInfoList(&list);
  bat_publishers->synopsisNormalizerInternal(
      &new_list, &list, 0);

  // simulate exclude and re-normalize
  new_list.erase(new_list.begin() + 3);
  ledger::PublisherInfoList new_list2;
  bat_publishers->synopsisNormalizerInternal(
      &new_list2, &new_list, 0);
  new_list2.erase(new_list2.begin() + 4);
  ledger::PublisherInfoList new_list3;
  bat_publishers->synopsisNormalizerInternal(
      &new_list3, &new_list2, 0);
  new_list3.erase(new_list3.begin() + 5);
  ledger::PublisherInfoList new_list4;
  bat_publishers->synopsisNormalizerInternal(
      &new_list4, &new_list3, 0);
  new_list4.erase(new_list4.begin() + 6);
  ledger::PublisherInfoList new_list5;
  bat_publishers->synopsisNormalizerInternal(
      &new_list5, &new_list4, 0);
  for (const auto& element : new_list5) {
    ASSERT_GE((int32_t)element->percent, 0);
    ASSERT_LE((int32_t)element->percent, 100);
  }
}

}  // namespace braveledger_publisher
