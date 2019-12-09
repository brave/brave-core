/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <memory>

#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/payments.h"

#include "base/time/time.h"
#include "base/i18n/time_formatting.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

namespace confirmations {

class ConfirmationsPaymentsTest : public ::testing::Test {
 protected:
  std::unique_ptr<MockConfirmationsClient> mock_confirmations_client_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;
  std::unique_ptr<Payments> payments_;

  ConfirmationsPaymentsTest() :
      mock_confirmations_client_(std::make_unique<MockConfirmationsClient>()),
      confirmations_(std::make_unique<ConfirmationsImpl>(
          mock_confirmations_client_.get())),
      payments_(std::make_unique<Payments>(confirmations_.get(),
          mock_confirmations_client_.get())) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsPaymentsTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
  base::Time GetNextPaymentUTCDate(
      const std::string& date,
      const std::string& next_token_redemption_date) {
    auto current_date = UTCDateFromString(date);

    auto token_redemption_date = UTCDateFromString(next_token_redemption_date);
    uint64_t token_redemption_date_in_seconds =
        token_redemption_date.ToDoubleT();

    return payments_->CalculateNextPaymentDate(current_date,
        token_redemption_date_in_seconds);
  }

  base::Time UTCDateFromString(
      const std::string& date) {
    const std::string utc_date = date + " 00:00:00 +00:00";

    base::Time time;
    if (!base::Time::FromString(utc_date.c_str(), &time)) {
      return base::Time();
    }

    return time;
  }
};

TEST_F(ConfirmationsPaymentsTest, InvalidJson_AsList) {
  // Arrange
  std::string json = "[{FOOBAR}]";

  // Act
  auto is_valid = payments_->SetFromJson(json);

  // Assert
  EXPECT_FALSE(is_valid);
}

TEST_F(ConfirmationsPaymentsTest, InvalidJson_AsDictionary) {
  // Arrange
  std::string json = "{FOOBAR}";

  // Act
  auto is_valid = payments_->SetFromJson(json);

  // Assert
  EXPECT_FALSE(is_valid);
}

TEST_F(ConfirmationsPaymentsTest, InvalidJson_DefaultBalance) {
  // Arrange
  std::string json = "[{\"balance\":\"INVALID\",\"month\":\"2019-06\",\"transactionCount\":\"10\"}]";  // NOLINT
  payments_->SetFromJson(json);

  // Act
  auto balance = payments_->GetBalance();

  // Assert
  EXPECT_EQ(0.0, balance);
}

TEST_F(ConfirmationsPaymentsTest, InvalidJsonWrongType_DefaultBalance) {
  // Arrange
  std::string json = "[{\"balance\":5,\"month\":\"2019-06\",\"transactionCount\":\"10\"}]";  // NOLINT
  payments_->SetFromJson(json);

  // Act
  auto balance = payments_->GetBalance();

  // Assert
  EXPECT_EQ(0.0, balance);
}

TEST_F(ConfirmationsPaymentsTest, InvalidJson_DefaultTransactionCount) {
  // Arrange
  std::string json = "[{\"balance\":\"0.5\",\"month\":\"2019-06\",\"transactionCount\":\"INVALID\"}]";  // NOLINT
  payments_->SetFromJson(json);

  auto date = UTCDateFromString("6 July 2019");

  // Act
  auto transaction_count = payments_->GetTransactionCountForMonth(date);

  // Assert
  EXPECT_EQ(0ULL, transaction_count);
}

TEST_F(ConfirmationsPaymentsTest, InvalidJsonWrongType_DefaultTransactionCount) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"0.5\",\"month\":\"2019-06\",\"transactionCount\":5}]";  // NOLINT
  payments_->SetFromJson(json);

  auto date = UTCDateFromString("6 July 2019");

  // Act
  auto transaction_count = payments_->GetTransactionCountForMonth(date);

  // Assert
  EXPECT_EQ(0ULL, transaction_count);
}

TEST_F(ConfirmationsPaymentsTest, Balance_ForSinglePayment) {
  // Arrange
  std::string json = "[{\"balance\":\"0.5\",\"month\":\"2019-06\",\"transactionCount\":\"10\"}]";  // NOLINT
  payments_->SetFromJson(json);

  // Act
  auto balance = payments_->GetBalance();

  // Assert
  EXPECT_EQ(0.5, balance);
}

TEST_F(ConfirmationsPaymentsTest, Balance_ForMultiplePayments) {
  // Arrange
  std::string json = "[{\"balance\":\"0.5\",\"month\":\"2019-06\",\"transactionCount\":\"10\"},{\"balance\":\"0.25\",\"month\":\"2019-05\",\"transactionCount\":\"5\"}]";  // NOLINT
  payments_->SetFromJson(json);

  // Act
  auto balance = payments_->GetBalance();

  // Assert
  EXPECT_EQ(0.75, balance);
}

TEST_F(ConfirmationsPaymentsTest, Balance_ForMultiplePayments_AscendingMonthOrder) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"0.25\",\"month\":\"2019-05\",\"transactionCount\":\"5\"},{\"balance\":\"0.5\",\"month\":\"2019-06\",\"transactionCount\":\"10\"}]";  // NOLINT
  payments_->SetFromJson(json);

  // Act
  auto balance = payments_->GetBalance();

  // Assert
  EXPECT_EQ(0.75, balance);
}

TEST_F(ConfirmationsPaymentsTest, TransactionCount_ForThisMonth) {
  // Arrange
  std::string json = "[{\"balance\":\"0.5\",\"month\":\"2019-06\",\"transactionCount\":\"10\"}]";  // NOLINT
  payments_->SetFromJson(json);

  auto date = UTCDateFromString("6 June 2019");

  // Act
  auto transaction_count = payments_->GetTransactionCountForMonth(date);

  // Assert
  EXPECT_EQ(10ULL, transaction_count);
}

TEST_F(ConfirmationsPaymentsTest, TransactionCount_ForThisMonthWithMultiplePayments) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"0.5\",\"month\":\"2019-06\",\"transactionCount\":\"10\"},{\"balance\":\"0.25\",\"month\":\"2019-05\",\"transactionCount\":\"5\"}]";  // NOLINT
  payments_->SetFromJson(json);

  auto date = UTCDateFromString("6 June 2019");

  // Act
  auto transaction_count = payments_->GetTransactionCountForMonth(date);

  // Assert
  EXPECT_EQ(10ULL, transaction_count);
}

TEST_F(ConfirmationsPaymentsTest, TransactionCount_ForThisMonthWithMultiplePayments_AscendingMonthOrder) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"0.25\",\"month\":\"2019-05\",\"transactionCount\":\"5\"},{\"balance\":\"0.5\",\"month\":\"2019-06\",\"transactionCount\":\"10\"}]";  // NOLINT
  payments_->SetFromJson(json);

  auto date = UTCDateFromString("6 June 2019");

  // Act
  auto transaction_count = payments_->GetTransactionCountForMonth(date);

  // Assert
  EXPECT_EQ(10ULL, transaction_count);
}

TEST_F(ConfirmationsPaymentsTest, CalculateNextPaymentDate_BeforeNextPaymentDate_RedeemTokensThisMonth_BalanceLastMonth) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"0\",\"month\":\"2019-07\",\"transactionCount\":\"0\"},{\"balance\":\"0.25\",\"month\":\"2019-06\",\"transactionCount\":\"5\"}]";  // NOLINT
  payments_->SetFromJson(json);

  std::string date = "3 July 2019";
  std::string next_token_redemption_date = "21 July 2019";

  // Act
  auto next_payment_date =
      GetNextPaymentUTCDate(date, next_token_redemption_date);

  // Assert
  auto expected_next_payment_date = UTCDateFromString("5 July 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(ConfirmationsPaymentsTest, CalculateNextPaymentDate_BeforeNextPaymentDate_RedeemTokensThisMonth_BalanceLastMonth_AscendingMonthOrder) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"0.25\",\"month\":\"2019-06\",\"transactionCount\":\"5\"},{\"balance\":\"0\",\"month\":\"2019-07\",\"transactionCount\":\"0\"}]";  // NOLINT
  payments_->SetFromJson(json);

  std::string date = "3 July 2019";
  std::string next_token_redemption_date = "21 July 2019";

  // Act
  auto next_payment_date =
      GetNextPaymentUTCDate(date, next_token_redemption_date);

  // Assert
  auto expected_next_payment_date = UTCDateFromString("5 July 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(ConfirmationsPaymentsTest, CalculateNextPaymentDate_BeforeNextPaymentDate_RedeemTokensThisMonth_DefaultBalanceLastMonth) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"1.5\",\"month\":\"2019-07\",\"transactionCount\":\"30\"}]";  // NOLINT
  payments_->SetFromJson(json);

  std::string date = "3 July 2019";
  std::string next_token_redemption_date = "21 July 2019";

  // Act
  auto next_payment_date =
      GetNextPaymentUTCDate(date, next_token_redemption_date);

  // Assert
  auto expected_next_payment_date = UTCDateFromString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(ConfirmationsPaymentsTest, CalculateNextPaymentDate_BeforeNextPaymentDate_RedeemTokensThisMonth_NoBalanceLastMonth) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"0\",\"month\":\"2019-06\",\"transactionCount\":\"0\"},{\"balance\":\"0\",\"month\":\"2019-05\",\"transactionCount\":\"0\"}]";  // NOLINT
  payments_->SetFromJson(json);

  std::string date = "3 July 2019";
  std::string next_token_redemption_date = "21 July 2019";

  // Act
  auto next_payment_date =
      GetNextPaymentUTCDate(date, next_token_redemption_date);

  // Assert
  auto expected_next_payment_date = UTCDateFromString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(ConfirmationsPaymentsTest, CalculateNextPaymentDate_BeforeNextPaymentDate_RedeemTokensThisMonth_NoBalanceLastMonth_AscendingMonthOrder) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"0\",\"month\":\"2019-05\",\"transactionCount\":\"0\"},{\"balance\":\"0\",\"month\":\"2019-06\",\"transactionCount\":\"0\"}]";  // NOLINT
  payments_->SetFromJson(json);

  std::string date = "3 July 2019";
  std::string next_token_redemption_date = "21 July 2019";

  // Act
  auto next_payment_date =
      GetNextPaymentUTCDate(date, next_token_redemption_date);

  // Assert
  auto expected_next_payment_date = UTCDateFromString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(ConfirmationsPaymentsTest, CalculateNextPaymentDate_AfterNextPaymentDate_RedeemTokensThisMonth_BalanceThisMonth) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"0.5\",\"month\":\"2019-07\",\"transactionCount\":\"10\"},{\"balance\":\"0\",\"month\":\"2019-06\",\"transactionCount\":\"0\"}]";  // NOLINT
  payments_->SetFromJson(json);

  std::string date = "15 July 2019";
  std::string next_token_redemption_date = "28 July 2019";

  // Act
  auto next_payment_date =
      GetNextPaymentUTCDate(date, next_token_redemption_date);

  // Assert
  auto expected_next_payment_date = UTCDateFromString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(ConfirmationsPaymentsTest, CalculateNextPaymentDate_AfterNextPaymentDate_RedeemTokensThisMonth_BalanceThisMonth_AscendingMonthOrder) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"0\",\"month\":\"2019-06\",\"transactionCount\":\"0\"},{\"balance\":\"0.5\",\"month\":\"2019-07\",\"transactionCount\":\"10\"}]";  // NOLINT
  payments_->SetFromJson(json);

  std::string date = "15 July 2019";
  std::string next_token_redemption_date = "28 July 2019";

  // Act
  auto next_payment_date =
      GetNextPaymentUTCDate(date, next_token_redemption_date);

  // Assert
  auto expected_next_payment_date = UTCDateFromString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(ConfirmationsPaymentsTest, CalculateNextPaymentDate_AfterNextPaymentDate_RedeemTokensThisMonth_DefaultBalanceThisMonth) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"0.25\",\"month\":\"2019-05\",\"transactionCount\":\"5\"}]";  // NOLINT
  payments_->SetFromJson(json);

  std::string date = "6 July 2019";
  std::string next_token_redemption_date = "15 July 2019";

  // Act
  auto next_payment_date =
      GetNextPaymentUTCDate(date, next_token_redemption_date);

  // Assert
  auto expected_next_payment_date = UTCDateFromString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(ConfirmationsPaymentsTest, CalculateNextPaymentDate_AfterNextPaymentDate_RedeemTokensThisMonth_NoBalanceThisMonth) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"0.0\",\"month\":\"2019-07\",\"transactionCount\":\"0\"},{\"balance\":\"1.75\",\"month\":\"2019-06\",\"transactionCount\":\"35\"}]";  // NOLINT
  payments_->SetFromJson(json);

  std::string date = "6 July 2019";
  std::string next_token_redemption_date = "15 July 2019";

  // Act
  auto next_payment_date =
      GetNextPaymentUTCDate(date, next_token_redemption_date);

  // Assert
  auto expected_next_payment_date = UTCDateFromString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(ConfirmationsPaymentsTest, CalculateNextPaymentDate_AfterNextPaymentDate_RedeemTokensThisMonth_NoBalanceThisMonth_AscendingMonthOrder) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"1.75\",\"month\":\"2019-06\",\"transactionCount\":\"35\"},{\"balance\":\"0.0\",\"month\":\"2019-07\",\"transactionCount\":\"0\"}]";  // NOLINT
  payments_->SetFromJson(json);

  std::string date = "6 July 2019";
  std::string next_token_redemption_date = "15 July 2019";

  // Act
  auto next_payment_date =
     GetNextPaymentUTCDate(date, next_token_redemption_date);

  // Assert
  auto expected_next_payment_date = UTCDateFromString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(ConfirmationsPaymentsTest, CalculateNextPaymentDate_AfterNextPaymentDate_RedeemTokensNextMonth_NoBalanceThisMonth) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"0\",\"month\":\"2019-07\",\"transactionCount\":\"0\"},{\"balance\":\"0.25\",\"month\":\"2019-06\",\"transactionCount\":\"5\"}]";  // NOLINT
  payments_->SetFromJson(json);

  std::string date = "6 July 2019";
  std::string next_token_redemption_date = "15 August 2019";

  // Act
  auto next_payment_date =
      GetNextPaymentUTCDate(date, next_token_redemption_date);

  // Assert
  auto expected_next_payment_date = UTCDateFromString("5 September 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(ConfirmationsPaymentsTest, CalculateNextPaymentDate_AfterNextPaymentDate_RedeemTokensNextMonth_NoBalanceThisMonth_AscendingMonthOrder) {  // NOLINT
  // Arrange
  std::string json = "[{\"balance\":\"0.25\",\"month\":\"2019-06\",\"transactionCount\":\"5\"},{\"balance\":\"0\",\"month\":\"2019-07\",\"transactionCount\":\"0\"}]";  // NOLINT
  payments_->SetFromJson(json);

  std::string date = "6 July 2019";
  std::string next_token_redemption_date = "15 August 2019";

  // Act
  auto next_payment_date =
      GetNextPaymentUTCDate(date, next_token_redemption_date);

  // Assert
  auto expected_next_payment_date = UTCDateFromString("5 September 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

}  // namespace confirmations
