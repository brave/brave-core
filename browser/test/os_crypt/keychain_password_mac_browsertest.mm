/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/os_crypt/sync/keychain_password_mac.h"
#include "content/public/test/browser_test.h"

namespace {
struct TestParams {
  const char* switch_to_append;
  const char* service_name;
  const char* account_name;
};

constexpr TestParams kTestVectors[] = {
    {
        nullptr,
        "Brave Safe Storage",
        "Brave",
    },
    {
        "import-brave",
        "Chromium Safe Storage",
        "Chromium",
    },
    {
        "import-chromium",
        "Chromium Safe Storage",
        "Chromium",
    },
    {
        "import-chrome",
        "Chrome Safe Storage",
        "Chrome",
    },
};
}  // namespace

class BraveKeychainPasswordTest
    : public InProcessBrowserTest,
      public ::testing::WithParamInterface<TestParams> {
 public:
  BraveKeychainPasswordTest() = default;
  BraveKeychainPasswordTest(const BraveKeychainPasswordTest&) = delete;
  BraveKeychainPasswordTest& operator=(const BraveKeychainPasswordTest&) =
      delete;
};

// It has to be browser test instead of unit test becasue GetServiceName() and
// GetAccountName() uses a static variable inside the function, only browser
// test can exit program for each test suite
IN_PROC_BROWSER_TEST_P(BraveKeychainPasswordTest, ServiceAndAccountName) {
  TestParams test_data(GetParam());
  if (test_data.switch_to_append) {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        test_data.switch_to_append);
  }
  EXPECT_STREQ(KeychainPassword::GetServiceName().c_str(),
               test_data.service_name);
  EXPECT_STREQ(KeychainPassword::GetAccountName().c_str(),
               test_data.account_name);
}

INSTANTIATE_TEST_SUITE_P(/*no prefix*/,
                         BraveKeychainPasswordTest,
                         testing::ValuesIn(kTestVectors));
