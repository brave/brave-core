/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "confirmations_client_mock.h"
#include "brave/vendor/bat-native-confirmations/src/confirmations_impl.h"

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=ConfirmationsTest.*

namespace bat_confirmations {

// Test data directory, relative to source root
const base::FilePath::CharType kTestDataRelativePath[] =
  FILE_PATH_LITERAL("brave/vendor/bat-native-confirmations/test/data");

class ConfirmationsTest : public ::testing::Test {
 protected:
  confirmations::MockConfirmationsClient *mock_confirmations_client;
  confirmations::ConfirmationsImpl* confirmations;

  ConfirmationsTest() :
      mock_confirmations_client(new confirmations::MockConfirmationsClient()),
      confirmations(new confirmations::ConfirmationsImpl(
          mock_confirmations_client)) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsTest() override {
    // You can do clean-up work that doesn't throw exceptions here
    delete confirmations;
    delete mock_confirmations_client;
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
  std::string GetMockDataPath(const std::string& filename) {
    base::FilePath path(kTestDataRelativePath);
    path = path.AppendASCII(filename);
    return path.value();
  }
};

TEST_F(ConfirmationsTest, DummyTest) {
  EXPECT_TRUE(true);
}

}  // namespace bat_confirmations
