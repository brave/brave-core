/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/task_environment.h"
#include "bat/ledger/internal/promotion/promotion.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"

// npm run test -- brave_unit_tests --filter=PromotionTest.*

using ::testing::_;
using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_promotion {

class PromotionTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;
 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<bat_ledger::MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<Promotion> promotion_;

  PromotionTest() {
      mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
      mock_ledger_impl_ = std::make_unique<bat_ledger::MockLedgerImpl>
          (mock_ledger_client_.get());
      promotion_ = std::make_unique<Promotion>(mock_ledger_impl_.get());
  }
};

TEST_F(PromotionTest, TestInitialize) {
  // Arrange
  EXPECT_CALL(*mock_ledger_impl_, GetAllPromotions(_));

  // Act
  promotion_->Initialize();

  // Assert
  // See Arrange
}

// void Fetch(ledger::FetchPromotionCallback callback);

TEST_F(PromotionTest, TestFetchWithNoWalletPaymentID) {
  // Arrange
  EXPECT_CALL(*mock_ledger_impl_,
      OnWalletProperties(ledger::Result::CORRUPTED_WALLET, _));
  const std::string payment_id = "";
  ON_CALL(*mock_ledger_impl_, GetPaymentId())
      .WillByDefault(testing::ReturnRef(payment_id));
  const std::string wallet_passphrase = "bob";
  ON_CALL(*mock_ledger_impl_, GetWalletPassphrase())
      .WillByDefault(testing::Return(wallet_passphrase));

  bool callback_called = false;

  ledger::FetchPromotionCallback fetch_promotion_callback =
      std::bind(
          [&callback_called](ledger::Result result,
              ledger::PromotionList promotions) {
            callback_called = true;
            EXPECT_EQ(result, ledger::Result::CORRUPTED_WALLET);
          },
      _1, _2);

  // Act
  promotion_->Fetch(fetch_promotion_callback);

  // Assert
  EXPECT_TRUE(callback_called);
}

TEST_F(PromotionTest, TestFetchWithNoWalletPassphrase) {
  // Arrange
  EXPECT_CALL(*mock_ledger_impl_,
      OnWalletProperties(ledger::Result::CORRUPTED_WALLET, _));
  const std::string payment_id = "bob";
  ON_CALL(*mock_ledger_impl_, GetPaymentId())
      .WillByDefault(testing::ReturnRef(payment_id));
  const std::string wallet_passphrase = "";
  ON_CALL(*mock_ledger_impl_, GetWalletPassphrase())
      .WillByDefault(testing::Return(wallet_passphrase));

  bool callback_called = false;

  ledger::FetchPromotionCallback fetch_promotion_callback =
      std::bind(
          [&callback_called](ledger::Result result,
              ledger::PromotionList promotions) {
            callback_called = true;
            EXPECT_EQ(result, ledger::Result::CORRUPTED_WALLET);
          },
      _1, _2);

  // Act
  promotion_->Fetch(fetch_promotion_callback);

  // Assert
  EXPECT_TRUE(callback_called);
}

TEST_F(PromotionTest, TestFetch) {
  // Arrange
  EXPECT_CALL(*mock_ledger_impl_, LoadURL(_, _, _, _, _, _));

  const std::string payment_id = "bob";
  ON_CALL(*mock_ledger_impl_, GetPaymentId())
      .WillByDefault(testing::ReturnRef(payment_id));

  const std::string wallet_passphrase = "fred";
  ON_CALL(*mock_ledger_impl_, GetWalletPassphrase())
      .WillByDefault(testing::Return(wallet_passphrase));

  ledger::FetchPromotionCallback fetch_promotion_callback =
      std::bind(
          [&](ledger::Result result,
              ledger::PromotionList promotions) {
          },
      _1, _2);

  // Act
  promotion_->Fetch(fetch_promotion_callback);
}

// void Claim(
//     const std::string& payload,
//     ledger::ClaimPromotionCallback callback);
// SUCCESS: Expect call Promotion::OnCompletedAttestation
// Nb. Needs a mock on AttestationImpl but cannot inject for testing

// void Attest(
//     const std::string& promotion_id,
//     const std::string& solution,
//     ledger::AttestPromotionCallback callback);
// Nb. Needs a mock on AttestationImpl but cannot inject for testing

// void Refresh(const bool retry_after_error);
TEST_F(PromotionTest, TestRefreshWithoutRetryWithTimerId) {
  // Arrange
  EXPECT_CALL(*mock_ledger_impl_, SetTimer(_, _)).Times(0);

  // Act
  promotion_->Refresh(false);
}

TEST_F(PromotionTest, TestRefreshWithRetryWithTimerId) {
  // Arrange
  EXPECT_CALL(*mock_ledger_impl_, SetTimer(_, _)).Times(0);

  // Act
  promotion_->Refresh(true);
}

TEST_F(PromotionTest, TestRefreshWithoutRetryOrTimerId) {
  // Arrange
  EXPECT_CALL(*mock_ledger_impl_, SetTimer(_, _)).Times(1);

  promotion_->SetLastCheckTimerIdForTesting(0);

  // Act
  promotion_->Refresh(false);
}

TEST_F(PromotionTest, TestRefreshWithRetryWithoutTimerId) {
  // Arrange
  EXPECT_CALL(*mock_ledger_impl_, SetTimer(_, _)).Times(1);
// calls ledger_->SetTimer with start_timer_in as a random value (how to test?)

  promotion_->SetLastCheckTimerIdForTesting(0);

  // Act
  promotion_->Refresh(true);
}

// void OnTimer(const uint32_t timer_id);
// Nb. Cannot test without mocking Promotion
//   discussed with tmancey and no value as code may be changing
//   need to confirm with nejc
// SUCCESS: timer_id == last_check_timer_id_;
// - expect call Fetch
// - last_check_timer_id_ = 0
// SUCCESS: timer_id == retry_timer_id_;
// - expect call ledger_->GetAllPromotions
// OR expect call Promotion::Retry
// FAIL: timer_id != (last_check_timer_id_ or retry_timer_id_)
// - neither expected (above) to be called


// void ClaimTokens(
//     ledger::PromotionPtr promotion,
//     ledger::ResultCallback callback);
TEST_F(PromotionTest, TestClaimTokensWithNullPromotion) {
  // Arrange
  bool callback_called = false;
  ledger::ResultCallback callback =
      std::bind(
        [&callback_called](ledger::Result result) {
          callback_called = (result == ledger::Result::LEDGER_ERROR);
        }, _1);

  // Act
  promotion_->ClaimTokens(nullptr, callback);

  // Assert
  EXPECT_TRUE(callback_called);
}

TEST_F(PromotionTest, TestClaimTokensWithPromotion) {
  // Arrange
  EXPECT_CALL(*mock_ledger_impl_, LoadURL(_, _, _, _, _, _));

  bool callback_called = false;

  ledger::PromotionPtr promotion = ledger::Promotion::New();
  promotion->id = "ABC123";
  promotion->suggestions = 1;

  ledger::ResultCallback callback =
      std::bind(
        [&callback_called](ledger::Result result) {
          callback_called = true;
        }, _1);

  const std::string payment_id = "bob";
  ON_CALL(*mock_ledger_impl_, GetPaymentId())
      .WillByDefault(testing::ReturnRef(payment_id));

  ledger::WalletInfoProperties wallet_info;
  wallet_info.key_info_seed.push_back(1);
  ON_CALL(*mock_ledger_impl_, GetWalletInfo())
      .WillByDefault(testing::ReturnRef(wallet_info));

  // Act
  promotion_->ClaimTokens(promotion.Clone(), callback);

  // Assert
  EXPECT_FALSE(callback_called);
}

}  // namespace braveledger_promotion
