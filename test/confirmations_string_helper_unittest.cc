/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "confirmations_client_mock.h"
#include "brave/vendor/bat-native-confirmations/src/confirmations_impl.h"
#include "brave/vendor/bat-native-confirmations/src/string_helper.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

namespace confirmations {

class ConfirmationsStringHelperTest : public ::testing::Test {
 protected:
  MockConfirmationsClient* mock_confirmations_client_;
  ConfirmationsImpl* confirmations_;

  ConfirmationsStringHelperTest() :
      mock_confirmations_client_(new MockConfirmationsClient()),
      confirmations_(new ConfirmationsImpl(mock_confirmations_client_)) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsStringHelperTest() override {
    // You can do clean-up work that doesn't throw exceptions here
    delete confirmations_;
    delete mock_confirmations_client_;
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

TEST_F(ConfirmationsStringHelperTest, DecodeHex) {
  // Arrange
  std::string hexadecimal = "3fc8ff3b121e7b7875750d26eaba6f06a3b06d96cf6b2fb898323917e7be9d16e255a4a6f7eb8647428f727c0d4e1958bd8e69a984eee38514d1e483aab27edf";  // NOLINT

  std::vector<uint8_t> valid_bytes = {
      0x3f, 0xc8, 0xff, 0x3b, 0x12, 0x1e, 0x7b, 0x78, 0x75, 0x75, 0x0d, 0x26,
      0xea, 0xba, 0x6f, 0x06, 0xa3, 0xb0, 0x6d, 0x96, 0xcf, 0x6b, 0x2f, 0xb8,
      0x98, 0x32, 0x39, 0x17, 0xe7, 0xbe, 0x9d, 0x16, 0xe2, 0x55, 0xa4, 0xa6,
      0xf7, 0xeb, 0x86, 0x47, 0x42, 0x8f, 0x72, 0x7c, 0x0d, 0x4e, 0x19, 0x58,
      0xbd, 0x8e, 0x69, 0xa9, 0x84, 0xee, 0xe3, 0x85, 0x14, 0xd1, 0xe4, 0x83,
      0xaa, 0xb2, 0x7e, 0xdf
  };

  // Act
  auto bytes = helper::String::decode_hex(hexadecimal);

  // Assert
  unsigned int index = 0;
  for (const auto& byte : bytes) {
    auto valid_byte = valid_bytes.at(index);
    if (byte != valid_byte) {
      FAIL();
    }

    index++;
  }

  SUCCEED();
}

}  // namespace confirmations
