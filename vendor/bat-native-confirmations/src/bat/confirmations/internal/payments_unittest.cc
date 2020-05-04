/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/payments.h"

#include <memory>
#include <string>

#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl.h"

#include "base/time/time.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatConfirmations*

using ::testing::NiceMock;

namespace confirmations {

class BatConfirmationsPaymentsTest : public ::testing::Test {
 protected:
  BatConfirmationsPaymentsTest()
      : confirmations_client_mock_(std::make_unique<
            NiceMock<ConfirmationsClientMock>>()),
        confirmations_(std::make_unique<ConfirmationsImpl>(
            confirmations_client_mock_.get())),
        payments_(std::make_unique<Payments>()) {
    // You can do set-up work for each test here
  }

  ~BatConfirmationsPaymentsTest() override {
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

  base::Time GetNextPaymentDate(
      const std::string& date,
      const std::string& next_token_redemption_date) {
    const base::Time time = TimeFromDateString(date);

    const base::Time token_redemption_time =
        TimeFromDateString(next_token_redemption_date);

    const uint64_t token_redemption_timestamp_in_seconds =
        token_redemption_time.ToDoubleT();

    return payments_->CalculateNextPaymentDate(time,
        token_redemption_timestamp_in_seconds);
  }

  base::Time TimeFromDateString(
      const std::string& date) {
    const std::string utc_date = date + " 23:59:59.999 +00:00";

    base::Time time;
    if (!base::Time::FromString(utc_date.c_str(), &time)) {
      return base::Time();
    }

    return time;
  }

  std::unique_ptr<ConfirmationsClientMock> confirmations_client_mock_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;
  std::unique_ptr<Payments> payments_;
};

TEST_F(BatConfirmationsPaymentsTest,
    InvalidJson) {
  // Arrange
  const std::string json = "[{FOOBAR}]";

  // Act
  const bool success = payments_->SetFromJson(json);

  // Assert
  EXPECT_FALSE(success);
}

TEST_F(BatConfirmationsPaymentsTest,
    Balance) {
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0.5",
        "month" : "2019-06",
        "transactionCount" : "10"
      }
    ]
  )";

  payments_->SetFromJson(json);

  // Act
  const double balance = payments_->GetBalance();

  // Assert
  EXPECT_EQ(0.5, balance);
}

TEST_F(BatConfirmationsPaymentsTest,
    BalanceAsInteger) {
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "5",
        "month" : "2019-06",
        "transactionCount" : "10"
      }
    ]
  )";

  payments_->SetFromJson(json);

  // Act
  const double balance = payments_->GetBalance();

  // Assert
  EXPECT_EQ(5, balance);
}

TEST_F(BatConfirmationsPaymentsTest,
    BalanceForMultiplePayments) {
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0.5",
        "month" : "2019-06",
        "transactionCount" : "10"
      },
      {
        "balance" : "0.25",
        "month" : "2019-05",
        "transactionCount" : "5"
      }
    ]
  )";

  payments_->SetFromJson(json);

  // Act
  const double balance = payments_->GetBalance();

  // Assert
  EXPECT_EQ(0.75, balance);
}

TEST_F(BatConfirmationsPaymentsTest,
    BalanceForMultiplePaymentsInAscendingOrder) {
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0.25",
        "month" : "2019-05",
        "transactionCount" : "5"
      },
      {
        "balance" : "0.5",
        "month" : "2019-06",
        "transactionCount" : "10"
      }
    ]
  )";

  payments_->SetFromJson(json);

  // Act
  const double balance = payments_->GetBalance();

  // Assert
  EXPECT_EQ(0.75, balance);
}

TEST_F(BatConfirmationsPaymentsTest,
    InvalidStringForBalance) {
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "INVALID",
        "month" : "2019-06",
        "transactionCount" : "10"
      }
    ]
  )";

  payments_->SetFromJson(json);

  // Act
  const double balance = payments_->GetBalance();

  // Assert
  EXPECT_EQ(0.0, balance);
}

TEST_F(BatConfirmationsPaymentsTest,
    InvalidTypeForBalance) {
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : 5,
        "month" : "2019-06",
        "transactionCount" : "10"
      }
    ]
  )";

  payments_->SetFromJson(json);

  // Act
  const double balance = payments_->GetBalance();

  // Assert
  EXPECT_EQ(0.0, balance);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIsBefore5thAndRedeemedTokensThisMonthWithBalanceLastMonth) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0",
        "month" : "2019-07",
        "transactionCount" : "0"
      },
      {
        "balance" : "0.25",
        "month" : "2019-06",
        "transactionCount" : "5"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "3 July 2019";
  const std::string next_token_redemption_date = "21 July 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 July 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIsBefore5thAndRedeemedTokensThisMonthWithBalanceLastMonthInAscendingOrder) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0.25",
        "month" : "2019-06",
        "transactionCount" : "5"
      },
      {
        "balance" : "0",
        "month" : "2019-07",
        "transactionCount" : "0"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "3 July 2019";
  const std::string next_token_redemption_date = "21 July 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 July 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIsBefore5thAndRedeemedTokensThisMonthWithMissingBalanceLastMonth) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "1.5",
        "month" : "2019-07",
        "transactionCount" : "30"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "3 July 2019";
  const std::string next_token_redemption_date = "21 July 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIsBefore5thAndRedeemedTokensThisMonthWithZeroBalanceLastMonth) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0",
        "month" : "2019-06",
        "transactionCount" : "0"
      },
      {
        "balance" : "0",
        "month" : "2019-05",
        "transactionCount" : "0"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "3 July 2019";
  const std::string next_token_redemption_date = "21 July 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIsBefore5thAndRedeemedTokensThisMonthWithZeroBalanceLastMonthInAscendingOrder) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0",
        "month" : "2019-05",
        "transactionCount" : "0"
      },
      {
        "balance" : "0",
        "month" : "2019-06",
        "transactionCount" : "0"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "3 July 2019";
  const std::string next_token_redemption_date = "21 July 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIs5thAndRedeemedTokensThisMonthWithBalanceLastMonth) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0",
        "month" : "2019-07",
        "transactionCount" : "0"
      },
      {
        "balance" : "0.25",
        "month" : "2019-06",
        "transactionCount" : "5"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "5 July 2019";
  const std::string next_token_redemption_date = "21 July 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 July 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIs5thAndRedeemedTokensThisMonthWithBalanceLastMonthInAscendingOrder) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0.25",
        "month" : "2019-06",
        "transactionCount" : "5"
      },
      {
        "balance" : "0",
        "month" : "2019-07",
        "transactionCount" : "0"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "5 July 2019";
  const std::string next_token_redemption_date = "21 July 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 July 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIs5thAndRedeemedTokensThisMonthWithMissingBalanceLastMonth) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "1.5",
        "month" : "2019-07",
        "transactionCount" : "30"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "5 July 2019";
  const std::string next_token_redemption_date = "21 July 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIs5thAndRedeemedTokensThisMonthWithZeroBalanceLastMonth) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0",
        "month" : "2019-06",
        "transactionCount" : "0"
      },
      {
        "balance" : "0",
        "month" : "2019-05",
        "transactionCount" : "0"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "5 July 2019";
  const std::string next_token_redemption_date = "21 July 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIs5thAndRedeemedTokensThisMonthWithZeroBalanceLastMonthInAscendingOrder) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0",
        "month" : "2019-05",
        "transactionCount" : "0"
      },
      {
        "balance" : "0",
        "month" : "2019-06",
        "transactionCount" : "0"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "5 July 2019";
  const std::string next_token_redemption_date = "21 July 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIsAfter5thAndRedeemedTokensThisMonthWithBalanceThisMonth) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0.5",
        "month" : "2019-07",
        "transactionCount" : "10"
      },
      {
        "balance" : "0",
        "month" : "2019-06",
        "transactionCount" : "0"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "15 July 2019";
  const std::string next_token_redemption_date = "28 July 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIsAfter5thAndRedeemedTokensThisMonthWithBalanceThisMonthInAscendingOrder) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0",
        "month" : "2019-06",
        "transactionCount" : "0"
      },
      {
        "balance" : "0.5",
        "month" : "2019-07",
        "transactionCount" : "10"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "15 July 2019";
  const std::string next_token_redemption_date = "28 July 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIsAfter5thAndRedeemedTokensThisMonthWithMissingBalanceThisMonth) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0.25",
        "month" : "2019-05",
        "transactionCount" : "5"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "6 July 2019";
  const std::string next_token_redemption_date = "15 July 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIsAfter5thAndRedeemedTokensThisMonthWithZeroBalanceThisMonth) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0.0",
        "month" : "2019-07",
        "transactionCount" : "0"
      },
      {
        "balance" : "1.75",
        "month" : "2019-06",
        "transactionCount" : "35"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "6 July 2019";
  const std::string next_token_redemption_date = "15 July 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIsAfter5thAndRedeemedTokensThisMonthWithZeroBalanceThisMonthInAscendingOrder) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "1.75",
        "month" : "2019-06",
        "transactionCount" : "35"
      },
      {
        "balance" : "0.0",
        "month" : "2019-07",
        "transactionCount" : "0"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "6 July 2019";
  const std::string next_token_redemption_date = "15 July 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 August 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIsAfter5thAndRedeemedTokensNextMonthWithZeroBalanceThisMonth) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0",
        "month" : "2019-07",
        "transactionCount" : "0"
      },
      {
        "balance" : "0.25",
        "month" : "2019-06",
        "transactionCount" : "5"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "6 July 2019";
  const std::string next_token_redemption_date = "15 August 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 September 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    NextPaymentDateIfDayIsAfter5thAndRedeemedTokensNextMonthWithZeroBalanceThisMonthInAscendingOrder) {  // NOLINT
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0.25",
        "month" : "2019-06",
        "transactionCount" : "5"
      },
      {
        "balance" : "0",
        "month" : "2019-07",
        "transactionCount" : "0"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const std::string date = "6 July 2019";
  const std::string next_token_redemption_date = "15 August 2019";

  // Act
  const base::Time next_payment_date =
      GetNextPaymentDate(date, next_token_redemption_date);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromDateString("5 September 2019");
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatConfirmationsPaymentsTest,
    TransactionCountForThisMonth) {
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0.5",
        "month" : "2019-06",
        "transactionCount" : "10"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const base::Time time = TimeFromDateString("6 June 2019");

  // Act
  const uint64_t transaction_count =
      payments_->GetTransactionCountForMonth(time);

  // Assert
  EXPECT_EQ(10UL, transaction_count);
}

TEST_F(BatConfirmationsPaymentsTest,
    TransactionCountForThisMonthWithMultiplePayments) {
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0.5",
        "month" : "2019-06",
        "transactionCount" : "10"
      },
      {
        "balance" : "0.25",
        "month" : "2019-05",
        "transactionCount" : "5"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const base::Time time = TimeFromDateString("6 June 2019");

  // Act
  const uint64_t transaction_count =
      payments_->GetTransactionCountForMonth(time);

  // Assert
  EXPECT_EQ(10UL, transaction_count);
}

TEST_F(BatConfirmationsPaymentsTest,
    TransactionCountForThisMonthWithMultiplePaymentsInAscendingOrder) {
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0.25",
        "month" : "2019-05",
        "transactionCount" : "5"
      },
      {
        "balance" : "0.5",
        "month" : "2019-06",
        "transactionCount" : "10"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const base::Time time = TimeFromDateString("6 June 2019");

  // Act
  const uint64_t transaction_count =
      payments_->GetTransactionCountForMonth(time);

  // Assert
  EXPECT_EQ(10UL, transaction_count);
}

TEST_F(BatConfirmationsPaymentsTest,
    InvalidValueForTransactionCount) {
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0.5",
        "month" : "2019-06",
        "transactionCount" : "INVALID"
      }
    ]
  )";

  payments_->SetFromJson(json);

  const base::Time time = TimeFromDateString("6 July 2019");

  // Act
  const uint64_t transaction_count =
      payments_->GetTransactionCountForMonth(time);

  // Assert
  EXPECT_EQ(0UL, transaction_count);
}

TEST_F(BatConfirmationsPaymentsTest,
    InvalidTypeForTransactionCount) {
  // Arrange
  const std::string json = R"(
    [
      {
        "balance" : "0.5",
        "month" : "2019-06",
        "transactionCount" : 5
      }
    ]
  )";

  payments_->SetFromJson(json);

  const base::Time time = TimeFromDateString("6 July 2019");

  // Act
  const uint64_t transaction_count =
      payments_->GetTransactionCountForMonth(time);

  // Assert
  EXPECT_EQ(0UL, transaction_count);
}

}  // namespace confirmations
