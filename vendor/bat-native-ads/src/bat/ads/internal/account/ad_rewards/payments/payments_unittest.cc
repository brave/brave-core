/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/ad_rewards/payments/payments.h"

#include <memory>

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsPaymentsTest : public UnitTestBase {
 protected:
  BatAdsPaymentsTest() : payments_(std::make_unique<Payments>()) {}

  ~BatAdsPaymentsTest() override = default;

  base::Time GetNextPaymentDate(const std::string& date,
                                const std::string& next_token_redemption_date) {
    const base::Time time = TimeFromDateString(date);

    const base::Time token_redemption_time =
        TimeFromDateString(next_token_redemption_date);

    return payments_->CalculateNextPaymentDate(time, token_redemption_time);
  }

  std::unique_ptr<Payments> payments_;
};

TEST_F(BatAdsPaymentsTest, InvalidJson) {
  // Arrange
  const std::string json = "[{FOOBAR}]";

  // Act
  const bool success = payments_->SetFromJson(json);

  // Assert
  EXPECT_FALSE(success);
}

TEST_F(BatAdsPaymentsTest, Balance) {
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

TEST_F(BatAdsPaymentsTest, BalanceAsInteger) {
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

TEST_F(BatAdsPaymentsTest, BalanceForMultiplePayments) {
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

TEST_F(BatAdsPaymentsTest, BalanceForMultiplePaymentsInAscendingOrder) {
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

TEST_F(BatAdsPaymentsTest, InvalidStringForBalance) {
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

TEST_F(BatAdsPaymentsTest, InvalidTypeForBalance) {
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(
    BatAdsPaymentsTest,
    NextPaymentDateIfDayIs5thAndRedeemedTokensThisMonthWithBalanceLastMonth) {
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(
    BatAdsPaymentsTest,
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

TEST_F(BatAdsPaymentsTest, TransactionCountForThisMonth) {
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
  const PaymentInfo payment = payments_->GetForMonth(time);

  // Assert
  EXPECT_EQ(10UL, payment.transaction_count);
}

TEST_F(BatAdsPaymentsTest, TransactionCountForThisMonthWithMultiplePayments) {
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
  const PaymentInfo payment = payments_->GetForMonth(time);

  // Assert
  EXPECT_EQ(10UL, payment.transaction_count);
}

TEST_F(BatAdsPaymentsTest,
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
  const PaymentInfo payment = payments_->GetForMonth(time);

  // Assert
  EXPECT_EQ(10UL, payment.transaction_count);
}

TEST_F(BatAdsPaymentsTest, InvalidValueForTransactionCount) {
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
  const PaymentInfo payment = payments_->GetForMonth(time);

  // Assert
  EXPECT_EQ(0UL, payment.transaction_count);
}

TEST_F(BatAdsPaymentsTest, InvalidTypeForTransactionCount) {
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
  const PaymentInfo payment = payments_->GetForMonth(time);

  // Assert
  EXPECT_EQ(0UL, payment.transaction_count);
}

}  // namespace ads
