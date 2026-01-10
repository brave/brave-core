// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/chart_code_plugin.h"

#include <string>

#include "base/test/values_test_util.h"
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

  auto result = chart_plugin.ValidateArtifact("chart", value);
  EXPECT_FALSE(result.has_value());
}

TEST(ChartCodePluginTest, ValidateArtifact_Failures) {
  ChartCodePlugin chart_plugin;

  // Not an object
  {
    auto value = base::Value("not an object");
    auto result = chart_plugin.ValidateArtifact("chart", value);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Chart must be an object");
  }

  // Missing data array
  {
    auto value = base::test::ParseJson(R"({"labels": {}})");
    auto result = chart_plugin.ValidateArtifact("chart", value);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Chart is missing 'data' array");
  }

  // Empty data array
  {
    auto value = base::test::ParseJson(R"({"data": []})");
    auto result = chart_plugin.ValidateArtifact("chart", value);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Chart has empty data array");
  }

  // Missing x field
  {
    auto value = base::test::ParseJson(R"({"data": [{"value": 10}]})");
    auto result = chart_plugin.ValidateArtifact("chart", value);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Chart data entry is missing required 'x' field");
  }

  // Invalid x field type
  {
    auto value =
        base::test::ParseJson(R"({"data": [{"x": true, "value": 10}]})");
    auto result = chart_plugin.ValidateArtifact("chart", value);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(),
              "Chart data entry 'x' field must be a string or number");
  }

  // Non-numeric value field
  {
    auto value = base::test::ParseJson(
        R"({"data": [{"x": "A", "value": "not a number"}]})");
    auto result = chart_plugin.ValidateArtifact("chart", value);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(),
              "Chart data entry values (except 'x') must be numbers");
  }

  // Invalid labels type
  {
    auto value = base::test::ParseJson(
        R"({"data": [{"x": "A", "value": 10}], "labels": "not an object"})");
    auto result = chart_plugin.ValidateArtifact("chart", value);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "Chart labels must be an object");
  }
}

}  // namespace ai_chat
