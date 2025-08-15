// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_script_utils.h"

#include <optional>

#include "testing/gtest/include/gtest/gtest.h"

namespace psst {

namespace {
constexpr char kTestScript[] = "console.log('test script');";
}  // namespace

class PsstScriptsUtilsUnitTest : public testing::Test {};

TEST_F(PsstScriptsUtilsUnitTest, GetScriptWithParamsCombinesParamsAndScript) {
  constexpr char kParamKey[] = "param1";
  constexpr char kParamValue[] = "value1";

  auto result = GetScriptWithParams(
      kTestScript,
      base::Value(base::Value::Dict().Set(kParamKey, kParamValue)));

  EXPECT_NE(result.find("const params ="), std::string::npos);
  EXPECT_NE(result.find("\"param1\": \"value1\""), std::string::npos);
  EXPECT_NE(result.find(kTestScript), std::string::npos);
}

TEST_F(PsstScriptsUtilsUnitTest, GetScriptWithParamsWrongCases) {
  EXPECT_EQ(GetScriptWithParams(kTestScript, std::nullopt), kTestScript);
  EXPECT_EQ(GetScriptWithParams(kTestScript, base::Value()), kTestScript);
}

}  // namespace psst
