// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/content/psst_script_utils.h"

#include <optional>

#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace psst {

namespace {
constexpr char kTestScript[] = "console.log('test script');";
}  // namespace

TEST(PsstScriptsUtilsUnitTest, GetScriptWithParamsCombinesParamsAndScript) {
  constexpr char kParamKey[] = "param1";
  constexpr char kParamValue[] = "value1";

  base::Value params(base::Value::Dict().Set(kParamKey, kParamValue));

  EXPECT_EQ(
      GetScriptWithParams(kTestScript, params.Clone()),
      base::StrCat({"const params = ",
                    base::WriteJsonWithOptions(
                        params.Clone(), base::JSONWriter::OPTIONS_PRETTY_PRINT)
                        .value(),
                    ";\n", kTestScript}));
}

TEST(PsstScriptsUtilsUnitTest, GetScriptWithParamsWrongCases) {
  EXPECT_EQ(GetScriptWithParams(kTestScript, std::nullopt), kTestScript);
  EXPECT_EQ(GetScriptWithParams(kTestScript, base::Value()), kTestScript);
}

}  // namespace psst
