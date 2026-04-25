/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/internal/current_test_util_internal.h"

#include "base/check.h"
#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_ads::test {

std::string GetUuidForCurrentTest() {
  const ::testing::TestInfo* const test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  CHECK(test_info);

  return base::ReplaceStringPlaceholders(
      "$1.$2", {test_info->test_suite_name(), test_info->name()}, nullptr);
}

std::string GetUuidForCurrentTestAndValue(const std::string& value) {
  return base::ReplaceStringPlaceholders(
      "$1:$2", {value, GetUuidForCurrentTest()}, nullptr);
}

}  // namespace brave_ads::test
