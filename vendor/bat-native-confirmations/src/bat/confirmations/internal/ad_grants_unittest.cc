/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <memory>

#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/ad_grants.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

namespace confirmations {

class ConfirmationsAdGrantsTest : public ::testing::Test {
 protected:
  std::unique_ptr<MockConfirmationsClient> mock_confirmations_client_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;
  std::unique_ptr<AdGrants> ad_grants_;

  ConfirmationsAdGrantsTest() :
      mock_confirmations_client_(std::make_unique<MockConfirmationsClient>()),
      confirmations_(std::make_unique<ConfirmationsImpl>(
          mock_confirmations_client_.get())),
      ad_grants_(std::make_unique<AdGrants>(confirmations_.get(),
          mock_confirmations_client_.get())) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsAdGrantsTest() override {
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
};

TEST_F(ConfirmationsAdGrantsTest, InvalidJson_AsDictionary) {
  // Arrange
  std::string json = "{FOOBAR}";

  // Act
  auto is_valid = ad_grants_->SetFromJson(json);

  // Assert
  EXPECT_FALSE(is_valid);
}

TEST_F(ConfirmationsAdGrantsTest, InvalidJson_DefaultBalance) {
  // Arrange
  std::string json = "{\"type\":\"ads\",\"amount\":\"INVALID\",\"lastClaim\":\"2019-06-13T12:14:46.150Z\"}";  // NOLINT
  ad_grants_->SetFromJson(json);

  // Act
  auto balance = ad_grants_->GetBalance();

  // Assert
  EXPECT_EQ(0.0, balance);
}

TEST_F(ConfirmationsAdGrantsTest, InvalidJsonWrongType_DefaultBalance) {
  // Arrange
  std::string json = "{\"type\":\"ads\",\"amount\":1,\"lastClaim\":\"2019-06-13T12:14:46.150Z\"}";  // NOLINT
  ad_grants_->SetFromJson(json);

  // Act
  auto balance = ad_grants_->GetBalance();

  // Assert
  EXPECT_EQ(0.0, balance);
}

TEST_F(ConfirmationsAdGrantsTest, Balance) {
  // Arrange
  std::string json = "{\"type\":\"ads\",\"amount\":\"5\",\"lastClaim\":\"2019-06-13T12:14:46.150Z\"}";  // NOLINT
  ad_grants_->SetFromJson(json);

  // Act
  auto balance = ad_grants_->GetBalance();

  // Assert
  EXPECT_EQ(5ULL, balance);
}

}  // namespace confirmations
