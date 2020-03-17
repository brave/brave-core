/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>

#include "bat/ledger/internal/common/bind_util.h"
#include "testing/gtest/include/gtest/gtest.h"
// npm run test -- brave_unit_tests --filter=BindUtilTest.*

namespace braveledger_bind_util {

class BindUtilTest : public ::testing::Test {
 protected:
  BindUtilTest() {
  }

  void SetUp() override {
  }
};

TEST_F(BindUtilTest, ExpectedStringFromContributionQueueToString) {
  const int8_t expected_publisher_count = 2;

  ledger::ContributionQueuePublisherList contribution_queue_publisher_list;

  for (int8_t publisher = 0; publisher < expected_publisher_count;
        publisher++) {
    ledger::ContributionQueuePublisherPtr contribution_queue_publisher =
        ledger::ContributionQueuePublisher::New();

    contribution_queue_publisher->publisher_key =
        std::to_string(1000 + publisher);
    contribution_queue_publisher->amount_percent =
        static_cast<double>(publisher * 25);
    contribution_queue_publisher_list.push_back(
        std::move(contribution_queue_publisher));
  }

  ledger::ContributionQueuePtr contribution_queue =
      ledger::ContributionQueue::New();
  contribution_queue->id = 1234;
  contribution_queue->type = ledger::RewardsType::RECURRING_TIP;
  contribution_queue->amount = 100.0;
  contribution_queue->partial = false;
  contribution_queue->publishers = std::move(contribution_queue_publisher_list);

  const std::string expected_contribution_queue_as_string = "{\"amount\":\"100.000000\",\"id\":\"1234\",\"partial\":false,\"publishers\":[{\"amount_percent\":\"0.000000\",\"publisher_key\":\"1000\"},{\"amount_percent\":\"25.000000\",\"publisher_key\":\"1001\"}],\"type\":16}";  // NOLINT 

  std::string contribution_queue_as_string =
      FromContributionQueueToString(std::move(contribution_queue));

  EXPECT_EQ(expected_contribution_queue_as_string,
      contribution_queue_as_string);
}

// ledger::ContributionQueuePtr FromStringToContributionQueue(
//     const std::string& data);
TEST_F(BindUtilTest, ExpectedContributionQueueFromStringToContributionQueue) {
  const std::string contribution_queue_as_string = "{\"amount\":\"100.000000\",\"id\":\"1234\",\"partial\":false,\"publishers\":[{\"amount_percent\":\"0.000000\",\"publisher_key\":\"1000\"},{\"amount_percent\":\"25.000000\",\"publisher_key\":\"1001\"}],\"type\":16}";  // NOLINT 

  const uint64_t expected_publisher_count = 2;

  ledger::ContributionQueuePtr contribution_queue =
      FromStringToContributionQueue(contribution_queue_as_string);

  EXPECT_EQ(contribution_queue->id, (uint64_t)1234);
  EXPECT_EQ(contribution_queue->type, ledger::RewardsType::RECURRING_TIP);
  EXPECT_EQ(contribution_queue->amount, 100.0);
  EXPECT_EQ(contribution_queue->partial, false);
  EXPECT_EQ(contribution_queue->publishers.size(), expected_publisher_count);

  int8_t publisher = 0;
  for (auto& contribution_queue_publisher : contribution_queue->publishers) {
    EXPECT_EQ(contribution_queue_publisher->publisher_key,
        std::to_string(1000 + publisher));
    EXPECT_EQ(contribution_queue_publisher->amount_percent,
        static_cast<double>(publisher * 25));
    publisher++;
  }
}

TEST_F(BindUtilTest,
    ContributionQueueTypeUnknownFromStringToContributionQueueWithInvalidType) {
  const std::string contribution_queue_as_string = "{\"amount\":\"100.000000\",\"id\":\"1234\",\"partial\":false,\"publishers\":[{\"amount_percent\":\"0.000000\",\"publisher_key\":\"1000\"},{\"amount_percent\":\"25.000000\",\"publisher_key\":\"1001\"}],\"type\":17}";  // NOLINT 

  ledger::ContributionQueuePtr contribution_queue =
      FromStringToContributionQueue(contribution_queue_as_string);

  EXPECT_EQ(contribution_queue->type, ledger::RewardsType::UNKNOWN);
}

// std::string FromPromotionToString(const ledger::PromotionPtr info);
TEST_F(BindUtilTest, ExpectedStringFromPromotionToString) {
  ledger::PromotionCredsPtr promotion_creds = ledger::PromotionCreds::New();
  promotion_creds->tokens = "ABC";
  promotion_creds->blinded_creds = "DEF";
  promotion_creds->signed_creds = "GHI";
  promotion_creds->public_key = "JKL";
  promotion_creds->batch_proof = "MNO";
  promotion_creds->claim_id = "PQR";

  ledger::PromotionPtr promotion = ledger::Promotion::New();
  promotion->id = "1234";
  promotion->version = 1;
  promotion->type = ledger::PromotionType::ADS;
  promotion->public_keys = "5678";
  promotion->suggestions = static_cast<int64_t>(1);
  promotion->approximate_value = 100.0;
  promotion->status = ledger::PromotionStatus::OVER;
  promotion->expires_at = static_cast<uint64_t>(2);
  promotion->claimed_at = static_cast<uint64_t>(3);
  promotion->legacy_claimed = false;
  promotion->credentials = std::move(promotion_creds);

  const std::string expected_promotion_as_string = "{\"approximate_value\":\"100.000000\",\"claimed_at\":\"3\",\"credentials\":{\"batch_proof\":\"MNO\",\"blinded_creds\":\"DEF\",\"claim_id\":\"PQR\",\"public_key\":\"JKL\",\"signed_creds\":\"GHI\",\"tokens\":\"ABC\"},\"expires_at\":\"2\",\"id\":\"1234\",\"legacy_claimed\":false,\"public_keys\":\"5678\",\"status\":5,\"suggestions\":1,\"type\":1,\"version\":1}";  // NOLINT 

  std::string promotion_as_string =
      FromPromotionToString(std::move(promotion));

  // Nb. if this test fails due to changes to Promotion or PromotionCreds,
  //     you must also maintain ExpectedPromotionFromStringToPromotion.
  EXPECT_EQ(expected_promotion_as_string,
      promotion_as_string);
}

// ledger::PromotionPtr FromStringToPromotion(const std::string& data);
TEST_F(BindUtilTest, ExpectedPromotionFromStringToPromotion) {
  const std::string promotion_as_string = "{\"approximate_value\":\"100.000000\",\"claimed_at\":\"3\",\"credentials\":{\"batch_proof\":\"MNO\",\"blinded_creds\":\"DEF\",\"claim_id\":\"PQR\",\"public_key\":\"JKL\",\"signed_creds\":\"GHI\",\"tokens\":\"ABC\"},\"expires_at\":\"2\",\"id\":\"1234\",\"legacy_claimed\":false,\"public_keys\":\"5678\",\"status\":5,\"suggestions\":1,\"type\":1,\"version\":1}";  // NOLINT 

  ledger::PromotionPtr promotion = FromStringToPromotion(promotion_as_string);

  EXPECT_EQ(promotion->id, "1234");
  EXPECT_EQ(promotion->version, static_cast<int64_t>(1));
  EXPECT_EQ(promotion->type, ledger::PromotionType::ADS);
  EXPECT_EQ(promotion->public_keys, "5678");
  EXPECT_EQ(promotion->suggestions, static_cast<int64_t>(1));
  EXPECT_EQ(promotion->approximate_value, 100.0);
  EXPECT_EQ(promotion->status, ledger::PromotionStatus::OVER);
  EXPECT_EQ(promotion->expires_at, static_cast<uint64_t>(2));
  EXPECT_EQ(promotion->claimed_at, static_cast<uint64_t>(3));
  EXPECT_EQ(promotion->legacy_claimed, false);
  EXPECT_EQ(promotion->credentials->tokens, "ABC");
  EXPECT_EQ(promotion->credentials->blinded_creds, "DEF");
  EXPECT_EQ(promotion->credentials->signed_creds, "GHI");
  EXPECT_EQ(promotion->credentials->public_key, "JKL");
  EXPECT_EQ(promotion->credentials->batch_proof, "MNO");
  EXPECT_EQ(promotion->credentials->claim_id, "PQR");
}

TEST_F(BindUtilTest, PromotionTypeUnknownFromStringToPromotionWithInvalidType) {
  const std::string promotion_as_string = "{\"approximate_value\":\"100.000000\",\"credentials\":{\"batch_proof\":\"MNO\",\"blinded_creds\":\"DEF\",\"claim_id\":\"PQR\",\"public_key\":\"JKL\",\"signed_creds\":\"GHI\",\"tokens\":\"ABC\"},\"expires_at\":\"2\",\"id\":\"1234\",\"legacy_claimed\":false,\"public_keys\":\"5678\",\"status\":5,\"suggestions\":1,\"type\":100,\"version\":1}";  // NOLINT 

  ledger::PromotionPtr promotion = FromStringToPromotion(promotion_as_string);

  EXPECT_EQ(promotion->type, ledger::PromotionType::UNKNOWN);
}

TEST_F(BindUtilTest,
    PromotionStatusUnknownFromStringToPromotionWithInvalidStatus) {
  const std::string promotion_as_string = "{\"approximate_value\":\"100.000000\",\"credentials\":{\"batch_proof\":\"MNO\",\"blinded_creds\":\"DEF\",\"claim_id\":\"PQR\",\"public_key\":\"JKL\",\"signed_creds\":\"GHI\",\"tokens\":\"ABC\"},\"expires_at\":\"2\",\"id\":\"1234\",\"legacy_claimed\":false,\"public_keys\":\"5678\",\"status\":-1,\"suggestions\":1,\"type\":1,\"version\":1}";  // NOLINT 

  ledger::PromotionPtr promotion = FromStringToPromotion(promotion_as_string);

  EXPECT_EQ(promotion->status, ledger::PromotionStatus::UNKNOWN);
}

}  // namespace braveledger_bind_util
