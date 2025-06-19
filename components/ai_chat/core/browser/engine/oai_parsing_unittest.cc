// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oai_parsing.h"

#include <memory>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/engine/engine_consumer.h"
#include "brave/components/ai_chat/core/browser/test_utils.h"
#include "brave/components/ai_chat/core/browser/tools/mock_tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class OaiParsingTest : public testing::Test {
 protected:
  // Helper function to create a base::Value::List from JSON string
  base::Value::List CreateToolCallsListFromJson(const std::string& json_str) {
    auto parsed_value = base::JSONReader::Read(json_str);
    EXPECT_TRUE(parsed_value.has_value())
        << "Failed to parse JSON: " << json_str;
    EXPECT_TRUE(parsed_value->is_list())
        << "Parsed value is not a list: " << parsed_value->type();
    return std::move(parsed_value->GetList());
  }
};

TEST_F(OaiParsingTest, ToolUseEventFromToolCallsResponse_ValidSingleToolCall) {
  // Test parsing a valid single tool call
  std::string json_str = R"([
    {
      "id": "call_123",
      "type": "function",
      "function": {
        "name": "get_weather",
        "arguments": "{\"location\":\"New York\"}"
      }
    }
  ])";

  base::Value::List tool_calls_list = CreateToolCallsListFromJson(json_str);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  auto expected = mojom::ToolUseEvent::New();
  expected->tool_id = "call_123";
  expected->tool_name = "get_weather";
  expected->input_json = "{\"location\":\"New York\"}";
  expected->output = std::nullopt;

  ExpectToolUseEventEquals(FROM_HERE, result[0], expected);
}

TEST_F(OaiParsingTest,
       ToolUseEventFromToolCallsResponse_ValidMultipleToolCalls) {
  // Test parsing multiple valid tool calls
  std::string json_str = R"([
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

  auto tool_calls_list = CreateToolCallsListFromJson(json_str);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  ASSERT_EQ(result.size(), 2u);

  // First tool call
  ExpectToolUseEventEquals(
      FROM_HERE, result[0],
      mojom::ToolUseEvent::New("get_weather", "call_123",
                               "{\"location\":\"New York\"}", std::nullopt));

  // Second tool call
  ExpectToolUseEventEquals(
      FROM_HERE, result[1],
      mojom::ToolUseEvent::New("search_web", "call_456",
                               "{\"query\":\"Hello, world!\"}", std::nullopt));
}

TEST_F(OaiParsingTest, ToolUseEventFromToolCallsResponse_MissingId) {
  // Test tool call without id field
  std::string json_str = R"([
    {
      "type": "function",
      "function": {
        "name": "get_weather",
        "arguments": "{\"location\":\"New York\"}"
      }
    }
  ])";

  auto tool_calls_list = CreateToolCallsListFromJson(json_str);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  ASSERT_EQ(result.size(), 0u);
}

TEST_F(OaiParsingTest, ToolUseEventFromToolCallsResponse_MissingFunctionName) {
  // Test tool call without function name
  std::string json_str = R"([
    {
      "id": "call_123",
      "type": "function",
      "function": {
        "arguments": "{\"location\":\"New York\"}"
      }
    }
  ])";

  auto tool_calls_list = CreateToolCallsListFromJson(json_str);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  ASSERT_EQ(result.size(), 0u);
}

TEST_F(OaiParsingTest, ToolUseEventFromToolCallsResponse_MissingArguments) {
  // Test tool call without function arguments
  std::string json_str = R"([
    {
      "id": "call_123",
      "type": "function",
      "function": {
        "name": "get_weather"
      }
    }
  ])";

  auto tool_calls_list = CreateToolCallsListFromJson(json_str);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  ASSERT_EQ(result.size(), 0u);
}

TEST_F(OaiParsingTest,
       ToolUseEventFromToolCallsResponse_MissingFunctionObject) {
  // Test tool call without function object - should be skipped
  std::string json_str = R"([
    {
      "id": "call_123",
      "type": "function"
    }
  ])";

  auto tool_calls_list = CreateToolCallsListFromJson(json_str);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  EXPECT_EQ(result.size(), 0u);
}

TEST_F(OaiParsingTest, ToolUseEventFromToolCallsResponse_InvalidToolCall) {
  // Test with non-dict tool call - should be skipped
  std::string json_str = R"([
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

  auto tool_calls_list = CreateToolCallsListFromJson(json_str);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  // Should only contain the valid tool call, invalid one should be skipped
  EXPECT_EQ(result.size(), 1u);
  ExpectToolUseEventEquals(
      FROM_HERE, result[0],
      mojom::ToolUseEvent::New("get_weather", "call_123",
                               "{\"location\":\"New York\"}", std::nullopt));
}

TEST_F(OaiParsingTest, ToolUseEventFromToolCallsResponse_EmptyList) {
  // Test with empty tool calls list
  std::string json_str = R"([])";

  auto tool_calls_list = CreateToolCallsListFromJson(json_str);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  EXPECT_EQ(result.size(), 0u);
}

// Tests for ToolApiDefinitionsFromTools

TEST_F(OaiParsingTest, ToolApiDefinitionsFromTools_EmptyTools) {
  const std::vector<const Tool*> tools;
  auto result = ToolApiDefinitionsFromTools(tools);
  EXPECT_FALSE(result.has_value());
}

TEST_F(OaiParsingTest, ToolApiDefinitionsFromTools_FunctionToolWithName) {
  auto mock_tool = std::make_unique<MockTool>("test_tool");
  std::vector<const Tool*> tools = {mock_tool.get()};

  auto result = ToolApiDefinitionsFromTools(tools);
  ASSERT_TRUE(result.has_value());

  std::string expected_json = R"([
    {
      "function": {
        "name": "test_tool"
      },
      "type": "function"
    }
  ])";

  EXPECT_THAT(*result, base::test::IsJson(expected_json));
}

TEST_F(OaiParsingTest,
       ToolApiDefinitionsFromTools_FunctionToolWithDescription) {
  auto mock_tool =
      std::make_unique<MockTool>("weather_tool", "Get weather information");
  std::vector<const Tool*> tools = {mock_tool.get()};

  auto result = ToolApiDefinitionsFromTools(tools);
  ASSERT_TRUE(result.has_value());

  std::string expected_json = R"([
    {
      "type": "function",
      "function": {
        "name": "weather_tool",
        "description": "Get weather information"
      }
    }
  ])";

  EXPECT_THAT(*result, base::test::IsJson(expected_json));
}

TEST_F(OaiParsingTest,
       ToolApiDefinitionsFromTools_FunctionToolWithInputProperties) {
  base::Value::Dict properties;
  // String property
  properties.Set("location", StringProperty("The location to get weather for"));
  // Object property - specific coordinates
  base::Value::Dict coordinates_properties;
  coordinates_properties.Set("latitude",
                             StringProperty("Latitude of the location"));
  coordinates_properties.Set("longitude",
                             StringProperty("Longitude of the location"));
  properties.Set("coordinates",
                 ObjectProperty("Coordinates of the location",
                                std::move(coordinates_properties)));
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
  std::vector<const Tool*> tools = {mock_tool.get()};

  auto result = ToolApiDefinitionsFromTools(tools);
  ASSERT_TRUE(result.has_value());

  std::string expected_json = R"([
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

  EXPECT_THAT(*result, base::test::IsJson(expected_json));
}

TEST_F(OaiParsingTest,
       ToolApiDefinitionsFromTools_FunctionToolWithRequiredProperties) {
  base::Value::Dict properties;
  properties.Set("location", StringProperty("The location to get weather for"));
  properties.Set("units", StringProperty("Temperature units"));

  std::vector<std::string> required_props = {"location"};
  auto mock_tool = std::make_unique<MockTool>(
      "weather_tool", "Get weather", "", std::move(properties), required_props);
  std::vector<const Tool*> tools = {mock_tool.get()};

  auto result = ToolApiDefinitionsFromTools(tools);
  ASSERT_TRUE(result.has_value());

  std::string expected_json = R"([
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
  EXPECT_THAT(*result, base::test::IsJson(expected_json));
}

TEST_F(OaiParsingTest,
       ToolApiDefinitionsFromTools_NonFunctionTypeWithExtraParams) {
  base::Value::Dict extra_params;
  extra_params.Set("width", 1920);
  extra_params.Set("height", 1080);
  auto mock_tool = std::make_unique<MockTool>(
      "screen_tool", "Screen capture", "computer_20241022", std::nullopt,
      std::nullopt, std::move(extra_params));
  std::vector<const Tool*> tools = {mock_tool.get()};

  auto result = ToolApiDefinitionsFromTools(tools);
  ASSERT_TRUE(result.has_value());

  std::string expected_json = R"([
    {
      "height": 1080,
      "name": "screen_tool",
      "type": "computer_20241022",
      "width": 1920
    }
  ])";
  EXPECT_THAT(*result, base::test::IsJson(expected_json));
}

TEST_F(OaiParsingTest, ToolApiDefinitionsFromTools_ToolWithEmptyName) {
  auto mock_tool1 = std::make_unique<MockTool>("");  // Empty name
  auto mock_tool2 = std::make_unique<MockTool>("valid_tool");
  std::vector<const Tool*> tools = {mock_tool1.get(), mock_tool2.get()};

  auto result = ToolApiDefinitionsFromTools(tools);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 1u);  // Only the valid tool should be included

  std::string expected_json = R"([
    {
      "function": {
        "name": "valid_tool"
      },
      "type": "function"
    }
  ])";
  EXPECT_THAT(*result, base::test::IsJson(expected_json));
}

TEST_F(OaiParsingTest, ToolApiDefinitionsFromTools_MultipleTools) {
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

  std::vector<const Tool*> tools = {function_tool.get(), custom_tool.get(),
                                 search_tool.get()};

  auto result = ToolApiDefinitionsFromTools(tools);
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 3u);

  std::string expected_json = R"([
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

  EXPECT_THAT(*result, base::test::IsJson(expected_json));
}

}  // namespace ai_chat
