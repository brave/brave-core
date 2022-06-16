/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/unittest/unittest_test_suite_util.h"

#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ads {

std::string GetCurrentTestSuiteAndName() {
  const ::testing::TestInfo* const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();

  return base::StringPrintf("%s.%s", test_info->test_suite_name(),
                            test_info->name());
}

std::string GetUuidForCurrentTestSuiteAndName(const std::string& name) {
  return base::StringPrintf("%s:%s", name.c_str(),
                            GetCurrentTestSuiteAndName().c_str());
}

}  // namespace ads
