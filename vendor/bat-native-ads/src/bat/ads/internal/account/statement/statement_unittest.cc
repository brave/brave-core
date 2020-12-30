/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/statement.h"

#include <memory>

#include "net/http/http_status_code.h"
#include "bat/ads/internal/account/ad_rewards/ad_rewards.h"
#include "bat/ads/internal/account/ad_rewards/ad_rewards_delegate_mock.h"
#include "bat/ads/internal/account/transactions/transactions.h"
#include "bat/ads/internal/account/wallet/wallet.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::NiceMock;

namespace ads {

class BatAdsStatementTest : public UnitTestBase {
 protected:
  BatAdsStatementTest()
      : ad_rewards_(std::make_unique<AdRewards>()),
        ad_rewards_delegate_mock_(std::make_unique<
            NiceMock<AdRewardsDelegateMock>>()),
        statement_(std::make_unique<Statement>(ad_rewards_.get())) {
    ad_rewards_->set_delegate(ad_rewards_delegate_mock_.get());
  }

  ~BatAdsStatementTest() override = default;

  privacy::UnblindedTokens* get_unblinded_payment_tokens() {
    return ConfirmationsState::Get()->get_unblinded_payment_tokens();
  }

  WalletInfo GetWallet() {
    Wallet wallet;
    wallet.Set("27a39b2f-9b2e-4eb0-bbb2-2f84447496e7",
        "x5uBvgI5MTTVY6sjGv65e9EHr8v7i+UxkFB9qVc5fP0=");

    return wallet.Get();
  }

  void AddTransactions(
      const int count) {
    for (int i = 0; i < count; i++) {
      ConfirmationInfo confirmation;
      confirmation.type = ConfirmationType::kViewed;

      transactions::Add(0.05, confirmation);
    }
  }

  std::unique_ptr<AdRewards> ad_rewards_;
  std::unique_ptr<AdRewardsDelegateMock> ad_rewards_delegate_mock_;
  std::unique_ptr<Statement> statement_;
};

TEST_F(BatAdsStatementTest,
    GetForThisMonth) {
  // Arrange
  const URLEndpoints endpoints = {
    {
      // Get payments
      R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_OK, R"(
            [
              {
                "balance": "0.35",
                "month": "2020-11",
                "transactionCount": "7"
              }
            ]
          )"
        }
      }
    },
    {
      // Get ad grants
      R"(/v1/promotions/ads/grants/summary?paymentId=27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_NO_CONTENT, ""
        }
      }
    }
  };

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnDidReconcileAdRewards()).Times(1);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnFailedToReconcileAdRewards()).Times(0);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnWillRetryToReconcileAdRewards()).Times(0);

  const WalletInfo wallet = GetWallet();
  ad_rewards_->MaybeReconcile(wallet);

  AdvanceClock(TimeFromDateString("18 November 2020"));
  AddTransactions(7);

  // Act
  const StatementInfo statement = statement_->Get(DistantPast(), Now());

  // Assert
  StatementInfo expected_statement;
  expected_statement.estimated_pending_rewards = 0.35;
  expected_statement.next_payment_date =
      TimeFromDateString("5 December 2020 23:59:59").ToDoubleT();
  expected_statement.ads_received_this_month = 7;
  expected_statement.earnings_this_month = 0.35;
  expected_statement.earnings_last_month = 0.0;
  expected_statement.transactions =
      transactions::GetCleared(DistantPast(), Now());
  expected_statement.uncleared_transactions = transactions::GetUncleared();

  EXPECT_EQ(expected_statement, statement);
}

TEST_F(BatAdsStatementTest,
    GetForThisMonthWithInternalServerErrorForPayments) {
  // Arrange
  const URLEndpoints endpoints = {
    {
      R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_INTERNAL_SERVER_ERROR, ""
        }
      }
    }
  };

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnDidReconcileAdRewards()).Times(0);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnFailedToReconcileAdRewards()).Times(1);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnWillRetryToReconcileAdRewards()).Times(1);

  const WalletInfo wallet = GetWallet();
  ad_rewards_->MaybeReconcile(wallet);

  AdvanceClock(TimeFromDateString("18 November 2020"));

  // Act
  const StatementInfo statement = statement_->Get(DistantPast(), Now());

  // Assert
  StatementInfo expected_statement;
  expected_statement.estimated_pending_rewards = 0.0;
  expected_statement.next_payment_date =
      TimeFromDateString("5 January 2021 23:59:59").ToDoubleT();
  expected_statement.ads_received_this_month = 0;
  expected_statement.earnings_this_month = 0.0;
  expected_statement.earnings_last_month = 0.0;
  expected_statement.transactions =
      transactions::GetCleared(DistantPast(), Now());
  expected_statement.uncleared_transactions = transactions::GetUncleared();

  EXPECT_EQ(expected_statement, statement);
}

TEST_F(BatAdsStatementTest,
    GetForThisMonthWithInvalidJsonForPayments) {
  // Arrange
  const URLEndpoints endpoints = {
    {
      R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_OK, "invalid_json"
        }
      }
    }
  };

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnDidReconcileAdRewards()).Times(0);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnFailedToReconcileAdRewards()).Times(1);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnWillRetryToReconcileAdRewards()).Times(1);

  const WalletInfo wallet = GetWallet();
  ad_rewards_->MaybeReconcile(wallet);

  AdvanceClock(TimeFromDateString("18 November 2020"));

  // Act
  const StatementInfo statement = statement_->Get(DistantPast(), Now());

  // Assert
  StatementInfo expected_statement;
  expected_statement.estimated_pending_rewards = 0.0;
  expected_statement.next_payment_date =
      TimeFromDateString("5 January 2021 23:59:59").ToDoubleT();
  expected_statement.ads_received_this_month = 0;
  expected_statement.earnings_this_month = 0.0;
  expected_statement.earnings_last_month = 0.0;
  expected_statement.transactions =
      transactions::GetCleared(DistantPast(), Now());
  expected_statement.uncleared_transactions = transactions::GetUncleared();

  EXPECT_EQ(expected_statement, statement);
}

TEST_F(BatAdsStatementTest,
    GetForThisMonthWithInternalServerErrorForAdGrants) {
  // Arrange
  const URLEndpoints endpoints = {
    {
      // Get payments
      R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_OK, R"(
            [
              {
                "balance": "0.35",
                "month": "2020-11",
                "transactionCount": "7"
              }
            ]
          )"
        }
      }
    },
    {
      // Get ad grants
      R"(/v1/promotions/ads/grants/summary?paymentId=27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_INTERNAL_SERVER_ERROR, ""
        }
      }
    }
  };

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnDidReconcileAdRewards()).Times(0);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnFailedToReconcileAdRewards()).Times(1);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnWillRetryToReconcileAdRewards()).Times(1);

  const WalletInfo wallet = GetWallet();
  ad_rewards_->MaybeReconcile(wallet);

  AdvanceClock(TimeFromDateString("18 November 2020"));
  AddTransactions(7);

  // Act
  const StatementInfo statement = statement_->Get(DistantPast(), Now());

  // Assert
  StatementInfo expected_statement;
  expected_statement.estimated_pending_rewards = 0.35;
  expected_statement.next_payment_date =
      TimeFromDateString("5 December 2020 23:59:59").ToDoubleT();
  expected_statement.ads_received_this_month = 7;
  expected_statement.earnings_this_month = 0.35;
  expected_statement.earnings_last_month = 0.0;
  expected_statement.transactions =
      transactions::GetCleared(DistantPast(), Now());
  expected_statement.uncleared_transactions = transactions::GetUncleared();

  EXPECT_EQ(expected_statement, statement);
}

TEST_F(BatAdsStatementTest,
    GetForThisMonthWithInvalidJsonForAdGrants) {
  // Arrange
  const URLEndpoints endpoints = {
    {
      // Get payments
      R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_OK, R"(
            [
              {
                "balance": "0.35",
                "month": "2020-11",
                "transactionCount": "7"
              }
            ]
          )"
        }
      }
    },
    {
      // Get ad grants
      R"(/v1/promotions/ads/grants/summary?paymentId=27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_OK, "invalid_json"
        }
      }
    }
  };

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnDidReconcileAdRewards()).Times(0);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnFailedToReconcileAdRewards()).Times(1);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnWillRetryToReconcileAdRewards()).Times(1);

  const WalletInfo wallet = GetWallet();
  ad_rewards_->MaybeReconcile(wallet);

  AdvanceClock(TimeFromDateString("18 November 2020"));
  AddTransactions(7);

  // Act
  const StatementInfo statement = statement_->Get(DistantPast(), Now());

  // Assert
  StatementInfo expected_statement;
  expected_statement.estimated_pending_rewards = 0.35;
  expected_statement.next_payment_date =
      TimeFromDateString("5 December 2020 23:59:59").ToDoubleT();
  expected_statement.ads_received_this_month = 7;
  expected_statement.earnings_this_month = 0.35;
  expected_statement.earnings_last_month = 0.0;
  expected_statement.transactions =
      transactions::GetCleared(DistantPast(), Now());
  expected_statement.uncleared_transactions = transactions::GetUncleared();

  EXPECT_EQ(expected_statement, statement);
}

TEST_F(BatAdsStatementTest,
    GetForPastTwoMonths) {
  // Arrange
  const URLEndpoints endpoints = {
    {
      // Get payments
      R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_OK, R"(
            [
              {
                "balance": "0.7",
                "month": "2020-10",
                "transactionCount": "14"
              },
              {
                "balance": "0.35",
                "month": "2020-11",
                "transactionCount": "7"
              }
            ]
          )"
        }
      }
    },
    {
      // Get ad grants
      R"(/v1/promotions/ads/grants/summary?paymentId=27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_NO_CONTENT, ""
        }
      }
    }
  };

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnDidReconcileAdRewards()).Times(1);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnFailedToReconcileAdRewards()).Times(0);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnWillRetryToReconcileAdRewards()).Times(0);

  const WalletInfo wallet = GetWallet();
  ad_rewards_->MaybeReconcile(wallet);

  AdvanceClock(TimeFromDateString("21 October 2020"));
  AddTransactions(14);
  AdvanceClock(TimeFromDateString("18 November 2020"));
  AddTransactions(7);

  // Act
  const StatementInfo statement = statement_->Get(DistantPast(), Now());

  // Assert
  StatementInfo expected_statement;
  expected_statement.estimated_pending_rewards = 1.05;
  expected_statement.next_payment_date =
      TimeFromDateString("5 December 2020 23:59:59").ToDoubleT();
  expected_statement.ads_received_this_month = 7;
  expected_statement.earnings_this_month = 0.35;
  expected_statement.earnings_last_month = 0.7;
  expected_statement.transactions =
      transactions::GetCleared(DistantPast(), Now());
  expected_statement.uncleared_transactions = transactions::GetUncleared();

  EXPECT_EQ(expected_statement, statement);
}

TEST_F(BatAdsStatementTest,
    GetForPastTwoMonthsOverTwoYears) {
  // Arrange
  const URLEndpoints endpoints = {
    {
      // Get payments
      R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_OK, R"(
            [
              {
                "balance": "0.7",
                "month": "2019-12",
                "transactionCount": "14"
              },
              {
                "balance": "0.35",
                "month": "2020-01",
                "transactionCount": "7"
              }
            ]
          )"
        }
      }
    },
    {
      // Get ad grants
      R"(/v1/promotions/ads/grants/summary?paymentId=27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_NO_CONTENT, ""
        }
      }
    }
  };

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnDidReconcileAdRewards()).Times(1);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnFailedToReconcileAdRewards()).Times(0);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnWillRetryToReconcileAdRewards()).Times(0);

  const WalletInfo wallet = GetWallet();
  ad_rewards_->MaybeReconcile(wallet);

  AdvanceClock(TimeFromDateString("25 December 2019"));
  AddTransactions(14);
  AdvanceClock(TimeFromDateString("1 January 2020"));
  AddTransactions(7);

  // Act
  const StatementInfo statement = statement_->Get(DistantPast(), Now());

  // Assert
  StatementInfo expected_statement;
  expected_statement.estimated_pending_rewards = 1.05;
  expected_statement.next_payment_date =
      TimeFromDateString("5 January 2020 23:59:59").ToDoubleT();
  expected_statement.ads_received_this_month = 7;
  expected_statement.earnings_this_month = 0.35;
  expected_statement.earnings_last_month = 0.7;
  expected_statement.transactions =
      transactions::GetCleared(DistantPast(), Now());
  expected_statement.uncleared_transactions = transactions::GetUncleared();

  EXPECT_EQ(expected_statement, statement);
}

TEST_F(BatAdsStatementTest,
    GetForPastThreeMonths) {
  // Arrange
  const URLEndpoints endpoints = {
    {
      // Get payments
      R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_OK, R"(
            [
              {
                "balance": "0.45",
                "month": "2020-09",
                "transactionCount": "9"
              },
              {
                "balance": "0.7",
                "month": "2020-10",
                "transactionCount": "14"
              },
              {
                "balance": "0.35",
                "month": "2020-11",
                "transactionCount": "7"
              }
            ]
          )"
        }
      }
    },
    {
      // Get ad grants
      R"(/v1/promotions/ads/grants/summary?paymentId=27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_NO_CONTENT, ""
        }
      }
    }
  };

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnDidReconcileAdRewards()).Times(1);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnFailedToReconcileAdRewards()).Times(0);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnWillRetryToReconcileAdRewards()).Times(0);

  const WalletInfo wallet = GetWallet();
  ad_rewards_->MaybeReconcile(wallet);

  AdvanceClock(TimeFromDateString("12 September 2020"));
  AddTransactions(9);
  AdvanceClock(TimeFromDateString("21 October 2020"));
  AddTransactions(14);
  AdvanceClock(TimeFromDateString("18 November 2020"));
  AddTransactions(7);

  // Act
  const StatementInfo statement = statement_->Get(DistantPast(), Now());

  // Assert
  StatementInfo expected_statement;
  expected_statement.estimated_pending_rewards = 1.5;
  expected_statement.next_payment_date =
      TimeFromDateString("5 December 2020 23:59:59").ToDoubleT();
  expected_statement.ads_received_this_month = 7;
  expected_statement.earnings_this_month = 0.35;
  expected_statement.earnings_last_month = 0.7;
  expected_statement.transactions =
      transactions::GetCleared(DistantPast(), Now());
  expected_statement.uncleared_transactions = transactions::GetUncleared();

  EXPECT_EQ(expected_statement, statement);
}

TEST_F(BatAdsStatementTest,
    GetForPastThreeMonthsWithAdGrants) {
  // Arrange
  const URLEndpoints endpoints = {
    {
      // Get payments
      R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_OK, R"(
            [
              {
                "balance": "0.45",
                "month": "2020-09",
                "transactionCount": "9"
              },
              {
                "balance": "0.7",
                "month": "2020-10",
                "transactionCount": "14"
              },
              {
                "balance": "0.35",
                "month": "2020-11",
                "transactionCount": "7"
              }
            ]
          )"
        }
      }
    },
    {
      // Get ad grants
      R"(/v1/promotions/ads/grants/summary?paymentId=27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_OK, R"(
            {
              "type" : "ads",
              "amount" : "0.75",
              "lastClaim" : "2020-11-18T12:34:56.789Z"
            }
          )"
        }
      }
    }
  };

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnDidReconcileAdRewards()).Times(1);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnFailedToReconcileAdRewards()).Times(0);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnWillRetryToReconcileAdRewards()).Times(0);

  const WalletInfo wallet = GetWallet();
  ad_rewards_->MaybeReconcile(wallet);

  AdvanceClock(TimeFromDateString("12 September 2020"));
  AddTransactions(9);
  AdvanceClock(TimeFromDateString("21 October 2020"));
  AddTransactions(14);
  AdvanceClock(TimeFromDateString("18 November 2020"));
  AddTransactions(7);

  // Act
  const StatementInfo statement = statement_->Get(DistantPast(), Now());

  // Assert
  StatementInfo expected_statement;
  expected_statement.estimated_pending_rewards = 0.75;
  expected_statement.next_payment_date =
      TimeFromDateString("5 December 2020 23:59:59").ToDoubleT();
  expected_statement.ads_received_this_month = 7;
  expected_statement.earnings_this_month = 0.35;
  expected_statement.earnings_last_month = 0.7;
  expected_statement.transactions =
      transactions::GetCleared(DistantPast(), Now());
  expected_statement.uncleared_transactions = transactions::GetUncleared();

  EXPECT_EQ(expected_statement, statement);
}

TEST_F(BatAdsStatementTest,
    KeepOldValuesAfterServerErrorForPayments) {
  // Arrange
  const URLEndpoints endpoints = {
    {
      // Get payments
      R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_OK, R"(
            [
              {
                "balance": "1.75",
                "month": "2020-10",
                "transactionCount": "35"
              },
              {
                "balance": "0.35",
                "month": "2020-11",
                "transactionCount": "7"
              }
            ]
          )"
        },
        {
          net::HTTP_INTERNAL_SERVER_ERROR, ""
        }
      }
    },
    {
      // Get ad grants
      R"(/v1/promotions/ads/grants/summary?paymentId=27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_NO_CONTENT, ""
        }
      }
    }
  };

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnDidReconcileAdRewards()).Times(1);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnFailedToReconcileAdRewards()).Times(1);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnWillRetryToReconcileAdRewards()).Times(1);

  const WalletInfo wallet = GetWallet();
  ad_rewards_->MaybeReconcile(wallet);
  ad_rewards_->MaybeReconcile(wallet);

  AdvanceClock(TimeFromDateString("21 October 2020"));
  AddTransactions(35);
  AdvanceClock(TimeFromDateString("18 November 2020"));
  AddTransactions(7);

  // Act
  const StatementInfo statement = statement_->Get(DistantPast(), Now());

  // Assert
  StatementInfo expected_statement;
  expected_statement.estimated_pending_rewards = 2.1;
  expected_statement.next_payment_date =
      TimeFromDateString("5 December 2020 23:59:59").ToDoubleT();
  expected_statement.ads_received_this_month = 7;
  expected_statement.earnings_this_month = 0.35;
  expected_statement.earnings_last_month = 1.75;
  expected_statement.transactions =
      transactions::GetCleared(DistantPast(), Now());
  expected_statement.uncleared_transactions = transactions::GetUncleared();

  EXPECT_EQ(expected_statement, statement);
}

TEST_F(BatAdsStatementTest,
    KeepOldValuesAfterServerErrorForAdGrants) {
  // Arrange
  const URLEndpoints endpoints = {
    {
      // Get payments
      R"(/v1/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_OK, R"(
            [
              {
                "balance": "1.75",
                "month": "2020-10",
                "transactionCount": "35"
              },
              {
                "balance": "0.35",
                "month": "2020-11",
                "transactionCount": "7"
              }
            ]
          )"
        },
        {
          net::HTTP_OK, R"(
            [
              {
                "balance": "1.75",
                "month": "2020-10",
                "transactionCount": "35"
              },
              {
                "balance": "0.35",
                "month": "2020-11",
                "transactionCount": "7"
              }
            ]
          )"
        }
      }
    },
    {
      // Get ad grants
      R"(/v1/promotions/ads/grants/summary?paymentId=27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)", {
        {
          net::HTTP_OK, R"(
            {
              "type" : "ads",
              "amount" : "0.75",
              "lastClaim" : "2020-11-18T12:34:56.789Z"
            }
          )"
        },
        {
          net::HTTP_INTERNAL_SERVER_ERROR, ""
        }
      }
    }
  };

  MockUrlRequest(ads_client_mock_, endpoints);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnDidReconcileAdRewards()).Times(1);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnFailedToReconcileAdRewards()).Times(1);

  EXPECT_CALL(*ad_rewards_delegate_mock_,
      OnWillRetryToReconcileAdRewards()).Times(1);

  const WalletInfo wallet = GetWallet();
  ad_rewards_->MaybeReconcile(wallet);
  ad_rewards_->MaybeReconcile(wallet);

  AdvanceClock(TimeFromDateString("21 October 2020"));
  AddTransactions(35);
  AdvanceClock(TimeFromDateString("18 November 2020"));
  AddTransactions(7);

  // Act
  const StatementInfo statement = statement_->Get(DistantPast(), Now());

  // Assert
  StatementInfo expected_statement;
  expected_statement.estimated_pending_rewards = 1.35;
  expected_statement.next_payment_date =
      TimeFromDateString("5 December 2020 23:59:59").ToDoubleT();
  expected_statement.ads_received_this_month = 7;
  expected_statement.earnings_this_month = 0.35;
  expected_statement.earnings_last_month = 1.75;
  expected_statement.transactions =
      transactions::GetCleared(DistantPast(), Now());
  expected_statement.uncleared_transactions = transactions::GetUncleared();

  EXPECT_EQ(expected_statement, statement);
}

}  // namespace ads
