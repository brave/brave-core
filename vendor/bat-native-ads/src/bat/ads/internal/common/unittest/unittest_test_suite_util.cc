/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/unittest/unittest_test_suite_util.h"

#include "base/check.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

namespace ads {

std::string GetNamespaceForCurrentTest() {
  const ::testing::TestInfo* const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  CHECK(test_info);

  return base::StringPrintf("%s.%s", test_info->test_suite_name(),
                            test_info->name());
}

std::string GetUuidForCurrentTestAndValue(const std::string& value) {
  return base::StringPrintf("%s:%s", value.c_str(),
                            GetNamespaceForCurrentTest().c_str());
}

}  // namespace ads
