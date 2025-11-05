// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oai_parsing.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/json/string_escape.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/mock_tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

TEST(OaiParsingTest, ToolUseEventFromToolCallsResponse_ValidSingleToolCall) {
  // Test parsing a valid single tool call
  constexpr char kToolCallsJson[] = R"([
    {
      "id": "call_123",
      "type": "function",
      "function": {
        "name": "get_weather",
        "arguments": "{\"location\":\"New York\"}"
      }
    }
  ])";

  base::Value::List tool_calls_list = base::test::ParseJsonList(kToolCallsJson);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  auto expected = mojom::ToolUseEvent::New(
      "get_weather", "call_123", "{\"location\":\"New York\"}", std::nullopt);

  EXPECT_MOJOM_EQ(result[0], expected);
}

TEST(OaiParsingTest, ToolUseEventFromToolCallsResponse_ValidMultipleToolCalls) {
  // Test parsing multiple valid tool calls
  constexpr char kToolCallsJson[] = R"([
    {
      "id": "call_123",
      "type": "function",
      "function": {
        "name": "get_weather",
        "arguments": "{\"location\":\"New York\"}"
      }
    },
    {
      "id": "call_456",
      "type": "function",
      "function": {
        "name": "search_web",
        "arguments": "{\"query\":\"Hello, world!\"}"
      }
    }
  ])";

  auto tool_calls_list = base::test::ParseJsonList(kToolCallsJson);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  ASSERT_EQ(result.size(), 2u);

  // First tool call
  EXPECT_MOJOM_EQ(result[0], mojom::ToolUseEvent::New(
                                 "get_weather", "call_123",
                                 "{\"location\":\"New York\"}", std::nullopt));

  // Second tool call
  EXPECT_MOJOM_EQ(
      result[1],
      mojom::ToolUseEvent::New("search_web", "call_456",
                               "{\"query\":\"Hello, world!\"}", std::nullopt));
}

TEST(OaiParsingTest, ToolUseEventFromToolCallsResponse_MissingId) {
  // Test tool call without id field, should be skipped
  constexpr char kToolCallsJson[] = R"([
    {
      "type": "function",
      "function": {
        "name": "get_weather",
        "arguments": "{\"location\":\"New York\"}"
      }
    }
  ])";

  auto tool_calls_list = base::test::ParseJsonList(kToolCallsJson);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  ASSERT_EQ(result.size(), 1u);

  EXPECT_MOJOM_EQ(result[0], mojom::ToolUseEvent::New(
                                 "get_weather", "",
                                 "{\"location\":\"New York\"}", std::nullopt));
}

TEST(OaiParsingTest, ToolUseEventFromToolCallsResponse_MissingFunctionName) {
  // Test tool call without function name, should be skipped
  constexpr char kToolCallsJson[] = R"([
    {
      "id": "call_123",
      "type": "function",
      "function": {
        "arguments": "{\"location\":\"New York\"}"
      }
    }
  ])";

  auto tool_calls_list = base::test::ParseJsonList(kToolCallsJson);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  ASSERT_EQ(result.size(), 1u);

  EXPECT_MOJOM_EQ(result[0], mojom::ToolUseEvent::New(
                                 "", "call_123", "{\"location\":\"New York\"}",
                                 std::nullopt));
}

TEST(OaiParsingTest, ToolUseEventFromToolCallsResponse_MissingFunctionObject) {
  // Test tool call without function object, should be skipped
  constexpr char kToolCallsJson[] = R"([
    {
      "id": "call_123",
      "type": "function"
    }
  ])";

  auto tool_calls_list = base::test::ParseJsonList(kToolCallsJson);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  EXPECT_EQ(result.size(), 0u);
}

TEST(OaiParsingTest, ToolUseEventFromToolCallsResponse_InvalidToolCall) {
  // Test with non-dict tool call, should be skipped
  constexpr char kToolCallsJson[] = R"([
    "invalid_string_entry",
    {
      "id": "call_123",
      "type": "function",
      "function": {
        "name": "get_weather",
        "arguments": "{\"location\":\"New York\"}"
      }
    }
  ])";

  auto tool_calls_list = base::test::ParseJsonList(kToolCallsJson);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  // Should only contain the valid tool call, invalid one should be skipped
  EXPECT_EQ(result.size(), 1u);

  EXPECT_MOJOM_EQ(result[0], mojom::ToolUseEvent::New(
                                 "get_weather", "call_123",
                                 "{\"location\":\"New York\"}", std::nullopt));
}

TEST(OaiParsingTest, ToolUseEventFromToolCallsResponse_EmptyList) {
  // Test with empty tool calls list
  constexpr char kToolCallsJson[] = R"([])";

  auto tool_calls_list = base::test::ParseJsonList(kToolCallsJson);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  EXPECT_EQ(result.size(), 0u);
}

// Tests for ToolApiDefinitionsFromTools

TEST(OaiParsingTest, ToolApiDefinitionsFromTools_EmptyTools) {
  std::vector<base::WeakPtr<Tool>> tools = {};
  auto result = ToolApiDefinitionsFromTools(tools);
  EXPECT_FALSE(result.has_value());
}

TEST(OaiParsingTest, ToolApiDefinitionsFromTools_FunctionToolWithName) {
  auto mock_tool = std::make_unique<MockTool>("test_tool");
  std::vector<base::WeakPtr<Tool>> tools = {mock_tool->GetWeakPtr()};

  auto result = ToolApiDefinitionsFromTools(tools);
  ASSERT_TRUE(result.has_value());

  constexpr char kExptectedJson[] = R"([
    {
      "function": {
        "name": "test_tool"
      },
      "type": "function"
    }
  ])";

  EXPECT_THAT(*result, base::test::IsJson(kExptectedJson));
}

TEST(OaiParsingTest, ToolApiDefinitionsFromTools_FunctionToolWithDescription) {
  auto mock_tool =
      std::make_unique<MockTool>("weather_tool", "Get weather information");
  std::vector<base::WeakPtr<Tool>> tools = {mock_tool->GetWeakPtr()};

  auto result = ToolApiDefinitionsFromTools(tools);
  ASSERT_TRUE(result.has_value());

  constexpr char kExptectedJson[] = R"([
    {
      "type": "function",
      "function": {
        "name": "weather_tool",
        "description": "Get weather information"
      }
    }
  ])";

  EXPECT_THAT(*result, base::test::IsJson(kExptectedJson));
}

TEST(OaiParsingTest,
     ToolApiDefinitionsFromTools_FunctionToolWithInputProperties) {
  base::Value::Dict properties;
  // String property
  properties.Set("location", StringProperty("The location to get weather for"));
  // Object property - specific coordinates
  properties.Set(
      "coordinates",
      ObjectProperty(
          "Coordinates of the location",
          {{"latitude", StringProperty("Latitude of the location")},
           {"longitude", StringProperty("Longitude of the location")}}));
  // Array property
  properties.Set("tags",
                 ArrayProperty("Tags for the weather query",
                               StringProperty("Tag for categorization")));
  // Boolean property
  properties.Set("include_forecast",
                 BooleanProperty("Whether to include forecast data"));
  // Number property
  properties.Set("max_results",
                 NumberProperty("Maximum number of results to return"));
  // Integr property
  properties.Set("priority", IntegerProperty("Priority of the request"));

  auto mock_tool = std::make_unique<MockTool>("weather_tool", "Get weather", "",
                                              std::move(properties));
  std::vector<base::WeakPtr<Tool>> tools = {mock_tool->GetWeakPtr()};

  auto result = ToolApiDefinitionsFromTools(tools);
  ASSERT_TRUE(result.has_value());

  constexpr char kExpectedJson[] = R"([
    {
      "type": "function",
      "function": {
        "name": "weather_tool",
        "description": "Get weather",
        "parameters": {
          "type": "object",
          "properties": {
            "location": {
              "description": "The location to get weather for",
              "type": "string"
            },
            "coordinates": {
              "description": "Coordinates of the location",
              "type": "object",
              "properties": {
                "latitude": {
                  "description": "Latitude of the location",
                  "type": "string"
                },
                "longitude": {
                  "description": "Longitude of the location",
                  "type": "string"
                }
              }
            },
            "tags": {
              "description": "Tags for the weather query",
              "type": "array",
              "items": {
                "type": "string",
                "description": "Tag for categorization"
              }
            },
            "include_forecast": {
              "description": "Whether to include forecast data",
              "type": "boolean"
            },
            "max_results": {
              "description": "Maximum number of results to return",
              "type": "number"
            },
            "priority": {
              "description": "Priority of the request",
              "type": "integer"
            }
          }
        }
      }
    }
  ])";

  EXPECT_THAT(*result, base::test::IsJson(kExpectedJson));
}

TEST(OaiParsingTest,
     ToolApiDefinitionsFromTools_FunctionToolWithRequiredProperties) {
  base::Value::Dict properties;
  properties.Set("location", StringProperty("The location to get weather for"));
  properties.Set("units", StringProperty("Temperature units"));

  std::vector<std::string> required_props = {"location"};
  auto mock_tool = std::make_unique<MockTool>(
      "weather_tool", "Get weather", "", std::move(properties), required_props);
  std::vector<base::WeakPtr<Tool>> tools = {mock_tool->GetWeakPtr()};

  auto result = ToolApiDefinitionsFromTools(tools);
  ASSERT_TRUE(result.has_value());

  constexpr char kExptectedJson[] = R"([
    {
      "function": {
        "description": "Get weather",
        "name": "weather_tool",
        "parameters": {
          "type": "object",
          "properties": {
            "location": {
              "type": "string",
              "description": "The location to get weather for"
            },
            "units": {
              "type": "string",
              "description": "Temperature units"
            }
          },
          "required": [
            "location"
          ]
        }
      },
      "type": "function"
    }
  ])";
  EXPECT_THAT(*result, base::test::IsJson(kExptectedJson));
}

TEST(OaiParsingTest,
     ToolApiDefinitionsFromTools_NonFunctionTypeWithExtraParams) {
  base::Value::Dict extra_params;
  extra_params.Set("width", 1920);
  extra_params.Set("height", 1080);
  auto mock_tool = std::make_unique<MockTool>(
      "screen_tool", "Screen capture", "computer_20241022", std::nullopt,
      std::nullopt, std::move(extra_params));
  std::vector<base::WeakPtr<Tool>> tools = {mock_tool->GetWeakPtr()};

  auto result = ToolApiDefinitionsFromTools(tools);
  ASSERT_TRUE(result.has_value());

  constexpr char kExptectedJson[] = R"([
    {
      "name": "screen_tool",
      "type": "computer_20241022",
      "width": 1920,
      "height": 1080
    }
  ])";
  EXPECT_THAT(*result, base::test::IsJson(kExptectedJson));
}

TEST(OaiParsingTest, ToolApiDefinitionsFromTools_FunctionTypeWithExtraParams) {
  // extra params should be ignored for function type tools (which is inferred
  // also if function type is empty)
  for (const std::string& function_type : {"function", ""}) {
    SCOPED_TRACE(testing::Message()
                 << "function type: "
                 << (function_type.empty() ? "[empty]" : function_type));
    base::Value::Dict extra_params;
    extra_params.Set("width", 1920);
    extra_params.Set("height", 1080);
    auto mock_tool = std::make_unique<MockTool>(
        "screen_tool", "Screen capture", function_type, std::nullopt,
        std::nullopt, std::move(extra_params));
    std::vector<base::WeakPtr<Tool>> tools = {mock_tool->GetWeakPtr()};

    auto result = ToolApiDefinitionsFromTools(tools);
    ASSERT_TRUE(result.has_value());

    constexpr char kExptectedJson[] = R"([
      {
        "type": "function",
        "function": {
          "name": "screen_tool",
          "description": "Screen capture"
        }
      }
    ])";
    EXPECT_THAT(*result, base::test::IsJson(kExptectedJson));
  }
  base::Value::Dict extra_params;
  extra_params.Set("width", 1920);
  extra_params.Set("height", 1080);
  auto mock_tool = std::make_unique<MockTool>(
      "screen_tool", "Screen capture", "function", std::nullopt, std::nullopt,
      std::move(extra_params));
  std::vector<base::WeakPtr<Tool>> tools = {mock_tool->GetWeakPtr()};

  auto result = ToolApiDefinitionsFromTools(tools);
  ASSERT_TRUE(result.has_value());

  constexpr char kExptectedJson[] = R"([
    {
      "type": "function",
      "function": {
        "name": "screen_tool",
        "description": "Screen capture"
      }
    }
  ])";
  EXPECT_THAT(*result, base::test::IsJson(kExptectedJson));
}

TEST(OaiParsingTest, ToolApiDefinitionsFromTools_ToolWithEmptyName) {
  auto mock_tool1 = std::make_unique<MockTool>("");  // Empty name
  auto mock_tool2 = std::make_unique<MockTool>("valid_tool");
  std::vector<base::WeakPtr<Tool>> tools = {mock_tool1->GetWeakPtr(),
                                            mock_tool2->GetWeakPtr()};

  auto result = ToolApiDefinitionsFromTools(tools);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1u);  // Only the valid tool should be included

  constexpr char kExptectedJson[] = R"([
    {
      "function": {
        "name": "valid_tool"
      },
      "type": "function"
    }
  ])";
  EXPECT_THAT(*result, base::test::IsJson(kExptectedJson));
}

TEST(OaiParsingTest, ToolApiDefinitionsFromTools_MultipleTools) {
  auto function_tool =
      std::make_unique<MockTool>("weather_tool", "Get weather");

  base::Value::Dict extra_params;
  extra_params.Set("screen_width", 1920);
  auto custom_tool = std::make_unique<MockTool>(
      "screen_tool", "", "computer_20241022", std::nullopt, std::nullopt,
      std::move(extra_params));

  base::Value::Dict properties;
  properties.Set("query", StringProperty(""));

  std::vector<std::string> required_props = {"query"};
  auto search_tool =
      std::make_unique<MockTool>("search_tool", "Search the web", "",
                                 std::move(properties), required_props);

  std::vector<base::WeakPtr<Tool>> tools = {function_tool->GetWeakPtr(),
                                            custom_tool->GetWeakPtr(),
                                            search_tool->GetWeakPtr()};

  auto result = ToolApiDefinitionsFromTools(tools);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 3u);

  constexpr char kExptectedJson[] = R"([
    {
      "function": {
        "description": "Get weather",
        "name": "weather_tool"
      },
      "type": "function"
    },
    {
      "name": "screen_tool",
      "screen_width": 1920,
      "type": "computer_20241022"
    },
    {
      "function": {
        "description": "Search the web",
        "name": "search_tool",
        "parameters": {
          "properties": {
            "query": {
              "type": "string"
            }
          },
          "required": [
            "query"
          ],
          "type": "object"
        }
      },
      "type": "function"
    }
  ])";

  EXPECT_THAT(*result, base::test::IsJson(kExptectedJson));
}

}  // namespace ai_chat
