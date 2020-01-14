/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>
#include <memory>

#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/string_helper.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

using std::placeholders::_1;

namespace confirmations {

class ConfirmationsStringHelperTest : public ::testing::Test {
 protected:
  std::unique_ptr<MockConfirmationsClient> mock_confirmations_client_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;

  ConfirmationsStringHelperTest() :
      mock_confirmations_client_(std::make_unique<MockConfirmationsClient>()),
      confirmations_(std::make_unique<ConfirmationsImpl>(
          mock_confirmations_client_.get())) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsStringHelperTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
    auto callback = std::bind(
        &ConfirmationsStringHelperTest::OnInitialize, this, _1);
    confirmations_->Initialize(callback);
  }

  void OnInitialize(const bool success) {
    EXPECT_EQ(true, success);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
};

TEST_F(ConfirmationsStringHelperTest, DecodeHex) {
  // Arrange
  std::string hexadecimal = "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a5661565033cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";  // NOLINT

  std::vector<uint8_t> private_key = {
      0xe9, 0xb1, 0xab, 0x4f, 0x44, 0xd3, 0x9e, 0xb0, 0x43, 0x23, 0x41, 0x1e,
      0xed, 0x0b, 0x5a, 0x2c, 0xee, 0xdf, 0xf0, 0x12, 0x64, 0x47, 0x4f, 0x86,
      0xe2, 0x9c, 0x70, 0x7a, 0x56, 0x61, 0x56, 0x50, 0x33, 0xce, 0xa0, 0x08,
      0x5c, 0xfd, 0x55, 0x1f, 0xaa, 0x17, 0x0c, 0x1d, 0xd7, 0xf6, 0xda, 0xaa,
      0x90, 0x3c, 0xdd, 0x31, 0x38, 0xd6, 0x1e, 0xd5, 0xab, 0x28, 0x45, 0xe2,
      0x24, 0xd5, 0x81, 0x44
  };

  // Act
  auto bytes = helper::String::decode_hex(hexadecimal);

  // Assert
  unsigned int index = 0;
  for (const auto& byte : bytes) {
    auto valid_byte = private_key.at(index);
    if (byte != valid_byte) {
      FAIL();
    }

    index++;
  }

  SUCCEED();
}

}  // namespace confirmations
