/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/ad_grants.h"

#include <memory>
#include <string>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl.h"

// npm run test -- brave_unit_tests --filter=BatConfirmations*

using ::testing::NiceMock;

namespace confirmations {

class BatConfirmationsAdGrantsTest : public ::testing::Test {
 protected:
  BatConfirmationsAdGrantsTest()
      : confirmations_client_mock_(std::make_unique<
            NiceMock<ConfirmationsClientMock>>()),
        confirmations_(std::make_unique<ConfirmationsImpl>(
            confirmations_client_mock_.get())),
        ad_grants_(std::make_unique<AdGrants>()) {
    // You can do set-up work for each test here
  }

  ~BatConfirmationsAdGrantsTest() override {
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

  std::unique_ptr<ConfirmationsClientMock> confirmations_client_mock_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;
  std::unique_ptr<AdGrants> ad_grants_;
};

TEST_F(BatConfirmationsAdGrantsTest,
    InvalidJson) {
  // Arrange
  const std::string json = "{FOOBAR}";

  // Act
  const bool is_valid = ad_grants_->SetFromJson(json);

  // Assert
  EXPECT_FALSE(is_valid);
}

TEST_F(BatConfirmationsAdGrantsTest,
    Amount) {
  // Arrange
  const std::string json = R"(
    {
      "type" : "ads",
      "amount" : "5.0",
      "lastClaim" : "2019-06-13T12:14:46.150Z"
    }
  )";

  ad_grants_->SetFromJson(json);

  // Act
  const double balance = ad_grants_->GetBalance();

  // Assert
  EXPECT_EQ(5.0, balance);
}

TEST_F(BatConfirmationsAdGrantsTest,
    AmountAsInteger) {
  // Arrange
  const std::string json = R"(
    {
      "type" : "ads",
      "amount" : "5",
      "lastClaim" : "2019-06-13T12:14:46.150Z"
    }
  )";

  ad_grants_->SetFromJson(json);

  // Act
  const double balance = ad_grants_->GetBalance();

  // Assert
  EXPECT_EQ(5.0, balance);
}

TEST_F(BatConfirmationsAdGrantsTest,
     InvalidStringForAmount) {
  // Arrange
  const std::string json = R"(
    {
      "type" : "ads",
      "amount" : "INVALID",
      "lastClaim" : "2019-06-13T12:14:46.150Z"
    }
  )";

  ad_grants_->SetFromJson(json);

  // Act
  const double balance = ad_grants_->GetBalance();

  // Assert
  EXPECT_EQ(0.0, balance);
}

TEST_F(BatConfirmationsAdGrantsTest,
    InvalidTypeForAmount) {
  // Arrange
  const std::string json = R"(
    {
      "type" : "ads",
      "amount" : 1,
      "lastClaim" : "2019-06-13T12:14:46.150Z"
    }
  )";

  ad_grants_->SetFromJson(json);

  // Act
  const double balance = ad_grants_->GetBalance();

  // Assert
  EXPECT_EQ(0.0, balance);
}

}  // namespace confirmations
