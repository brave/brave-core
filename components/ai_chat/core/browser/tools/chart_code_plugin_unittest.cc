// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/chart_code_plugin.h"

#include <string_view>

#include "base/test/values_test_util.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

TEST(ChartCodePluginTest, ValidateArtifact_Success) {
  ChartCodePlugin chart_plugin;
  auto value = base::test::ParseJson(R"json({
    "data": [
      {"x": "Jan", "sales": 100, "profit": 30},
      {"x": "Feb", "sales": 150, "profit": 45}
    ],
    "labels": {
      "sales": "Sales ($)",
      "profit": "Profit ($)"
    }
  })json");

  auto result = chart_plugin.ValidateArtifact(value);
  EXPECT_FALSE(result.has_value());
}

TEST(ChartCodePluginTest, ValidateArtifact_Failures) {
  ChartCodePlugin chart_plugin;

  struct TestCase {
    std::string_view name;
    std::string_view input;
    std::string_view expected_error;
  } cases[] = {
      {"Not an object", "\"not an object\"", "Chart must be an object"},
      {"Missing data array", R"({"labels": {}})",
       "Chart is missing 'data' array"},
      {"Empty data array", R"({"data": []})", "Chart has empty data array"},
      {"Missing x field", R"({"data": [{"value": 10}]})",
       "Chart data entry is missing required 'x' field"},
      {"Invalid x field type", R"({"data": [{"x": true, "value": 10}]})",
       "Chart data entry 'x' field must be a string or number"},
      {"Non-numeric value field",
       R"({"data": [{"x": "A", "value": "not a number"}]})",
       "Chart data entry values (except 'x') must be numbers"},
      {"Invalid labels type",
       R"({"data": [{"x": "A", "value": 10}], "labels": "not an object"})",
       "Chart labels must be an object"},
      {"Unexpected chart keys",
       R"({"data": [{"x": "A", "value": 10}], "extra": {}})",
       "Chart may only contain 'data' and optional 'labels' fields"},
  };

  for (auto& tc : cases) {
    SCOPED_TRACE(tc.name);
    auto input = base::test::ParseJson(tc.input);
    auto result = chart_plugin.ValidateArtifact(input);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), tc.expected_error);
  }
}

}  // namespace ai_chat
