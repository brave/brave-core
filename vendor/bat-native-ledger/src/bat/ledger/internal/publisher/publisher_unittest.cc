/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <iostream>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/database/database_mock.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/publisher/publisher.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::Invoke;

// npm run test -- brave_unit_tests --filter=PublisherTest.*

namespace braveledger_publisher {

class PublisherTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  void CreatePublisherInfoList(ledger::PublisherInfoList* list) {
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

  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<bat_ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<Publisher> publisher_;
  std::unique_ptr<braveledger_database::MockDatabase> mock_database_;

  PublisherTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<bat_ledger::MockLedgerImpl>(mock_ledger_client_.get());
    publisher_ = std::make_unique<Publisher>(mock_ledger_impl_.get());
    mock_database_ = std::make_unique<braveledger_database::MockDatabase>(
        mock_ledger_impl_.get());
  }

  void SetUp() override {
    ON_CALL(*mock_ledger_impl_, database())
      .WillByDefault(testing::Return(mock_database_.get()));

    ON_CALL(*mock_ledger_client_, GetDoubleState(ledger::kStateScoreA))
      .WillByDefault(
          Invoke([this](const std::string& key) {
            return a_;
          }));

    ON_CALL(*mock_ledger_client_, GetDoubleState(ledger::kStateScoreB))
      .WillByDefault(
          Invoke([this](const std::string& key) {
            return b_;
          }));

    ON_CALL(*mock_ledger_client_, SetDoubleState(_, _))
      .WillByDefault(
        Invoke([this](
            const std::string& key,
            double value) {
          if (key == ledger::kStateScoreA) {
            a_ = value;
            return;
          }

          if (key == ledger::kStateScoreB) {
            b_ = value;
            return;
          }
        }));
  }

  double a_ = 0;
  double b_ = 0;
};

TEST_F(PublisherTest, CalcScoreConsts5) {
  publisher_->CalcScoreConsts(5);

  ASSERT_EQ(a_, 14500);
  ASSERT_EQ(b_, -14000);
}

TEST_F(PublisherTest, CalcScoreConsts8) {
  publisher_->CalcScoreConsts(8);

  ASSERT_EQ(a_, 14200);
  ASSERT_EQ(b_, -13400);
}

TEST_F(PublisherTest, CalcScoreConsts60) {
  publisher_->CalcScoreConsts(60);

  ASSERT_EQ(a_, 9000);
  ASSERT_EQ(b_, -3000);
}

TEST_F(PublisherTest, concaveScore) {
  publisher_->CalcScoreConsts(5);
  EXPECT_NEAR(publisher_->concaveScore(5), 1, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(15), 1.06285, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(60), 1.28703, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(1000), 3.15289, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(10000), 8.80133, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(150000), 32.6498, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(500000), 59.2068, 0.001f);

  publisher_->CalcScoreConsts(8);
  EXPECT_NEAR(publisher_->concaveScore(5), 0.979606, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(15), 1.04477, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(60), 1.27505, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(1000), 3.16717, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(10000), 8.8769, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(150000), 32.9766, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(500000), 59.8128, 0.001f);

  publisher_->CalcScoreConsts(60);
  EXPECT_NEAR(publisher_->concaveScore(5), 0.455342, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(15), 0.607625, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(60), 1, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(1000), 3.50416, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(10000), 10.7089, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(150000), 40.9918, 0.001f);
  EXPECT_NEAR(publisher_->concaveScore(500000), 74.7025, 0.001f);
}

TEST_F(PublisherTest, synopsisNormalizerInternal) {
  // create test PublisherInfo list
  ledger::PublisherInfoList new_list;
  ledger::PublisherInfoList list;
  CreatePublisherInfoList(&list);
  publisher_->synopsisNormalizerInternal(
      &new_list, &list, 0);

  // simulate exclude and re-normalize
  new_list.erase(new_list.begin() + 3);
  ledger::PublisherInfoList new_list2;
  publisher_->synopsisNormalizerInternal(
      &new_list2, &new_list, 0);
  new_list2.erase(new_list2.begin() + 4);
  ledger::PublisherInfoList new_list3;
  publisher_->synopsisNormalizerInternal(
      &new_list3, &new_list2, 0);
  new_list3.erase(new_list3.begin() + 5);
  ledger::PublisherInfoList new_list4;
  publisher_->synopsisNormalizerInternal(
      &new_list4, &new_list3, 0);
  new_list4.erase(new_list4.begin() + 6);
  ledger::PublisherInfoList new_list5;
  publisher_->synopsisNormalizerInternal(
      &new_list5, &new_list4, 0);
  for (const auto& element : new_list5) {
    ASSERT_GE((int32_t)element->percent, 0);
    ASSERT_LE((int32_t)element->percent, 100);
  }
}

}  // namespace braveledger_publisher
