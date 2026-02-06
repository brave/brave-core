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
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

constexpr char kDefaultFaviconUrl[] =
    "chrome-untrusted://resources/brave-icons/globe.svg";

// Tests for ParseToolCallRequest

TEST(OaiParsingTest, ParseToolCallRequest_ValidToolCall) {
  constexpr char kToolCallJson[] = R"({
    "id": "call_123",
    "type": "function",
    "function": {
      "name": "get_weather",
      "arguments": "{\"location\":\"New York\"}"
    }
  })";

  auto tool_call = base::test::ParseJsonDict(kToolCallJson);
  auto result = ParseToolCallRequest(tool_call);

  ASSERT_TRUE(result.has_value());

  auto expected = mojom::ToolUseEvent::New("get_weather", "call_123",
                                           "{\"location\":\"New York\"}",
                                           std::nullopt, nullptr, false);

  EXPECT_MOJOM_EQ(*result, expected);
}

TEST(OaiParsingTest, ParseToolCallRequest_MissingId) {
  constexpr char kToolCallJson[] = R"({
    "type": "function",
    "function": {
      "name": "get_weather",
      "arguments": "{\"location\":\"New York\"}"
    }
  })";

  auto tool_call = base::test::ParseJsonDict(kToolCallJson);
  auto result = ParseToolCallRequest(tool_call);

  ASSERT_TRUE(result.has_value());

  EXPECT_MOJOM_EQ(*result, mojom::ToolUseEvent::New(
                               "get_weather", "", "{\"location\":\"New York\"}",
                               std::nullopt, nullptr, false));
}

TEST(OaiParsingTest, ParseToolCallRequest_MissingFunctionName) {
  constexpr char kToolCallJson[] = R"({
    "id": "call_123",
    "type": "function",
    "function": {
      "arguments": "{\"location\":\"New York\"}"
    }
  })";

  auto tool_call = base::test::ParseJsonDict(kToolCallJson);
  auto result = ParseToolCallRequest(tool_call);

  ASSERT_TRUE(result.has_value());

  EXPECT_MOJOM_EQ(*result, mojom::ToolUseEvent::New(
                               "", "call_123", "{\"location\":\"New York\"}",
                               std::nullopt, nullptr, false));
}

TEST(OaiParsingTest, ParseToolCallRequest_MissingFunctionObject) {
  // Test tool call without function object, should be skipped
  constexpr char kToolCallJson[] = R"({
    "id": "call_123",
    "type": "function"
  })";

  auto tool_call = base::test::ParseJsonDict(kToolCallJson);
  auto result = ParseToolCallRequest(tool_call);

  EXPECT_FALSE(result.has_value());
}

TEST(OaiParsingTest, ParseToolCallRequest_AlignmentCheck) {
  // Test parsing alignment_check in tool calls covering all scenarios
  constexpr char kToolCallsJson[] = R"([
    {
      "id": "no_check",
      "type": "function",
      "function": {
        "name": "allowed_by_default",
        "arguments": "{}"
      }
    },
    {
      "id": "explicit_allow",
      "type": "function",
      "function": {
        "name": "explicitly_allowed",
        "arguments": "{}"
      },
      "alignment_check": {
        "allowed": true,
        "reasoning": "This is fine"
      }
    },
    {
      "id": "deny_with_reason",
      "type": "function",
      "function": {
        "name": "denied_tool",
        "arguments": "{}"
      },
      "alignment_check": {
        "allowed": false,
        "reasoning": "Security risk"
      }
    },
    {
      "id": "deny_no_reason",
      "type": "function",
      "function": {
        "name": "denied_no_explanation",
        "arguments": "{}"
      },
      "alignment_check": {
        "allowed": false
      }
    },
    {
      "id": "malformed_check",
      "type": "function",
      "function": {
        "name": "invalid_alignment",
        "arguments": "{}"
      },
      "alignment_check": {
        "some_field": "value"
      }
    },
    {
      "id": "empty_check",
      "type": "function",
      "function": {
        "name": "empty_alignment",
        "arguments": "{}"
      },
      "alignment_check": {}
    }
  ])";

  auto tool_calls_list = base::test::ParseJsonList(kToolCallsJson);
  ASSERT_EQ(tool_calls_list.size(), 6u);

  auto result0 = ParseToolCallRequest(tool_calls_list[0].GetDict());
  ASSERT_TRUE(result0.has_value());
  EXPECT_MOJOM_EQ(
      *result0, mojom::ToolUseEvent::New("allowed_by_default", "no_check", "{}",
                                         std::nullopt, nullptr, false))
      << "No alignment_check should result in no PermissionChallenge";

  auto result1 = ParseToolCallRequest(tool_calls_list[1].GetDict());
  ASSERT_TRUE(result1.has_value());
  EXPECT_MOJOM_EQ(
      *result1, mojom::ToolUseEvent::New("explicitly_allowed", "explicit_allow",
                                         "{}", std::nullopt, nullptr, false))
      << "alignment_check.allowed=true should not create PermissionChallenge";

  auto result2 = ParseToolCallRequest(tool_calls_list[2].GetDict());
  ASSERT_TRUE(result2.has_value());
  EXPECT_MOJOM_EQ(
      *result2,
      mojom::ToolUseEvent::New(
          "denied_tool", "deny_with_reason", "{}", std::nullopt,
          mojom::PermissionChallenge::New("Security risk", std::nullopt),
          false))
      << "alignment_check.allowed=false with reasoning should create "
         "PermissionChallenge with reasoning";

  auto result3 = ParseToolCallRequest(tool_calls_list[3].GetDict());
  ASSERT_TRUE(result3.has_value());
  EXPECT_MOJOM_EQ(
      *result3,
      mojom::ToolUseEvent::New(
          "denied_no_explanation", "deny_no_reason", "{}", std::nullopt,
          mojom::PermissionChallenge::New(std::nullopt, std::nullopt), false))
      << "alignment_check.allowed=false without reasoning should create "
         "PermissionChallenge with null reasoning";

  auto result4 = ParseToolCallRequest(tool_calls_list[4].GetDict());
  ASSERT_TRUE(result4.has_value());
  EXPECT_MOJOM_EQ(
      *result4, mojom::ToolUseEvent::New("invalid_alignment", "malformed_check",
                                         "{}", std::nullopt, nullptr, false))
      << "alignment_check without allowed field should be treated as allowed, "
         "no PermissionChallenge";

  auto result5 = ParseToolCallRequest(tool_calls_list[5].GetDict());
  ASSERT_TRUE(result5.has_value());
  EXPECT_MOJOM_EQ(
      *result5, mojom::ToolUseEvent::New("empty_alignment", "empty_check", "{}",
                                         std::nullopt, nullptr, false))
      << "Empty alignment_check should be treated as allowed, no "
         "PermissionChallenge";
}

// Tests for ToolUseEventFromToolCallsResponse
// Note: Single tool call parsing is tested via ParseToolCallRequest tests.
// These tests focus on list-level behavior.

TEST(OaiParsingTest, ToolUseEventFromToolCallsResponse_MultipleToolCalls) {
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
  EXPECT_MOJOM_EQ(result[0],
                  mojom::ToolUseEvent::New("get_weather", "call_123",
                                           "{\"location\":\"New York\"}",
                                           std::nullopt, nullptr, false));

  // Second tool call
  EXPECT_MOJOM_EQ(result[1],
                  mojom::ToolUseEvent::New("search_web", "call_456",
                                           "{\"query\":\"Hello, world!\"}",
                                           std::nullopt, nullptr, false));
}

TEST(OaiParsingTest, ToolUseEventFromToolCallsResponse_SkipsInvalidEntries) {
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

  EXPECT_MOJOM_EQ(result[0],
                  mojom::ToolUseEvent::New("get_weather", "call_123",
                                           "{\"location\":\"New York\"}",
                                           std::nullopt, nullptr, false));
}

TEST(OaiParsingTest, ToolUseEventFromToolCallsResponse_EmptyList) {
  // Test with empty tool calls list
  constexpr char kToolCallsJson[] = R"([])";

  auto tool_calls_list = base::test::ParseJsonList(kToolCallsJson);

  auto result = ToolUseEventFromToolCallsResponse(&tool_calls_list);

  EXPECT_EQ(result.size(), 0u);
}

// Tests for ParseToolCallResult

TEST(OaiParsingTest, ParseToolCallResult_ValidTextOutput) {
  constexpr char kToolResultJson[] = R"({
    "id": "call_123",
    "output_content": [
      {
        "type": "text",
        "text": "The weather is sunny"
      }
    ]
  })";

  auto tool_result = base::test::ParseJsonDict(kToolResultJson);
  auto result = ParseToolCallResult(tool_result);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE((*result)->output.has_value());
  ASSERT_EQ((*result)->output->size(), 1u);
  ASSERT_TRUE((*result)->output->at(0)->is_text_content_block());
  EXPECT_EQ((*result)->output->at(0)->get_text_content_block()->text,
            "The weather is sunny");
}

TEST(OaiParsingTest, ParseToolCallResult_MissingId) {
  constexpr char kToolResultJson[] = R"({
    "output_content": [
      {
        "type": "text",
        "text": "Result"
      }
    ]
  })";

  auto tool_result = base::test::ParseJsonDict(kToolResultJson);
  auto result = ParseToolCallResult(tool_result);

  EXPECT_FALSE(result.has_value());
}

TEST(OaiParsingTest, ParseToolCallResult_MissingOutputContent) {
  constexpr char kToolResultJson[] = R"({
    "id": "call_123"
  })";

  auto tool_result = base::test::ParseJsonDict(kToolResultJson);
  auto result = ParseToolCallResult(tool_result);

  EXPECT_FALSE(result.has_value());
}

TEST(OaiParsingTest, ParseToolCallResult_WebSourcesOutput) {
  constexpr char kToolResultJson[] = R"({
    "id": "call_search",
    "output_content": [
      {
        "type": "brave-chat.webSources",
        "sources": [
          {
            "title": "Example Site",
            "url": "https://example.com",
            "favicon": "https://imgs.search.brave.com/icon.png"
          }
        ],
        "query": "test search"
      }
    ]
  })";

  auto tool_result = base::test::ParseJsonDict(kToolResultJson);
  auto result = ParseToolCallResult(tool_result);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ((*result)->id, "call_search");
  ASSERT_TRUE((*result)->output.has_value());
  ASSERT_EQ((*result)->output->size(), 1u);
  ASSERT_TRUE((*result)->output->at(0)->is_web_sources_content_block());

  const auto& web_sources =
      (*result)->output->at(0)->get_web_sources_content_block();
  EXPECT_EQ(web_sources->query, "test search");
  ASSERT_EQ(web_sources->sources.size(), 1u);
  EXPECT_EQ(web_sources->sources[0]->title, "Example Site");
  EXPECT_EQ(web_sources->sources[0]->url.spec(), "https://example.com/");
  EXPECT_EQ(web_sources->sources[0]->favicon_url.spec(),
            "https://imgs.search.brave.com/icon.png");
}

TEST(OaiParsingTest, ParseToolCallResult_MixedOutputContent) {
  constexpr char kToolResultJson[] = R"({
    "id": "call_mixed",
    "output_content": [
      {
        "type": "text",
        "text": "Search results:"
      },
      {
        "type": "brave-chat.webSources",
        "sources": [
          {
            "title": "Result 1",
            "url": "https://example.com/1"
          }
        ]
      },
      {
        "type": "text",
        "text": "More text"
      }
    ]
  })";

  auto tool_result = base::test::ParseJsonDict(kToolResultJson);
  auto result = ParseToolCallResult(tool_result);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE((*result)->output.has_value());
  ASSERT_EQ((*result)->output->size(), 3u);

  ASSERT_TRUE((*result)->output->at(0)->is_text_content_block());
  EXPECT_EQ((*result)->output->at(0)->get_text_content_block()->text,
            "Search results:");

  ASSERT_TRUE((*result)->output->at(1)->is_web_sources_content_block());
  const auto& web_sources =
      (*result)->output->at(1)->get_web_sources_content_block();
  ASSERT_EQ(web_sources->sources.size(), 1u);
  EXPECT_EQ(web_sources->sources[0]->title, "Result 1");
  EXPECT_EQ(web_sources->sources[0]->url.spec(), "https://example.com/1");
  EXPECT_EQ(web_sources->sources[0]->favicon_url.spec(), kDefaultFaviconUrl);
  EXPECT_EQ(web_sources->query, std::nullopt);

  ASSERT_TRUE((*result)->output->at(2)->is_text_content_block());
  EXPECT_EQ((*result)->output->at(2)->get_text_content_block()->text,
            "More text");
}

TEST(OaiParsingTest, ParseToolCallResult_UnsupportedTypeSerializedAsText) {
  constexpr char kToolResultJson[] = R"({
    "id": "call_custom",
    "output_content": [
      {
        "type": "custom_type",
        "data": "some value"
      }
    ]
  })";

  auto tool_result = base::test::ParseJsonDict(kToolResultJson);
  auto result = ParseToolCallResult(tool_result);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE((*result)->output.has_value());
  ASSERT_EQ((*result)->output->size(), 1u);
  // Unsupported types get serialized as JSON text
  ASSERT_TRUE((*result)->output->at(0)->is_text_content_block());
  EXPECT_THAT(
      (*result)->output->at(0)->get_text_content_block()->text,
      base::test::IsJson(R"({"type":"custom_type","data":"some value"})"));
}

// Tests for ParseContentBlockFromDict

TEST(OaiParsingTest, ParseContentBlockFromDict_TextType) {
  constexpr char kBlockJson[] = R"({
    "type": "text",
    "text": "Hello, world!"
  })";

  auto block = base::test::ParseJsonDict(kBlockJson);
  auto result = ParseContentBlockFromDict(block);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE((*result)->is_text_content_block());
  EXPECT_EQ((*result)->get_text_content_block()->text, "Hello, world!");
}

TEST(OaiParsingTest, ParseContentBlockFromDict_TextTypeMissingText) {
  constexpr char kBlockJson[] = R"({
    "type": "text"
  })";

  auto block = base::test::ParseJsonDict(kBlockJson);
  auto result = ParseContentBlockFromDict(block);

  EXPECT_FALSE(result.has_value());
}

TEST(OaiParsingTest, ParseContentBlockFromDict_WebSourcesType) {
  constexpr char kBlockJson[] = R"({
    "type": "brave-chat.webSources",
    "sources": [
      {
        "title": "Site 1",
        "url": "https://site1.com",
        "favicon": "https://imgs.search.brave.com/favicon1.png"
      },
      {
        "title": "Site 2",
        "url": "https://site2.com"
      }
    ],
    "query": "search query"
  })";

  auto block = base::test::ParseJsonDict(kBlockJson);
  auto result = ParseContentBlockFromDict(block);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE((*result)->is_web_sources_content_block());

  const auto& web_sources = (*result)->get_web_sources_content_block();
  EXPECT_EQ(web_sources->query, "search query");
  ASSERT_EQ(web_sources->sources.size(), 2u);
  EXPECT_EQ(web_sources->sources[0]->title, "Site 1");
  EXPECT_EQ(web_sources->sources[0]->url.spec(), "https://site1.com/");
  EXPECT_EQ(web_sources->sources[0]->favicon_url.spec(),
            "https://imgs.search.brave.com/favicon1.png");
  EXPECT_EQ(web_sources->sources[1]->title, "Site 2");
  // Second source should have default favicon
  EXPECT_EQ(web_sources->sources[1]->favicon_url.spec(), kDefaultFaviconUrl);
}

TEST(OaiParsingTest, ParseContentBlockFromDict_WebSourcesInvalidFavicon) {
  // Favicon from disallowed host should cause source to be skipped
  constexpr char kBlockJson[] = R"({
    "type": "brave-chat.webSources",
    "sources": [
      {
        "title": "Valid Source",
        "url": "https://valid.com",
        "favicon": "https://imgs.search.brave.com/valid.png"
      },
      {
        "title": "Invalid Favicon Host",
        "url": "https://invalid.com",
        "favicon": "https://evil.com/favicon.png"
      },
      {
        "title": "Invalid Favicon Scheme",
        "url": "https://another.com",
        "favicon": "http://imgs.search.brave.com/insecure.png"
      }
    ]
  })";

  auto block = base::test::ParseJsonDict(kBlockJson);
  auto result = ParseContentBlockFromDict(block);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE((*result)->is_web_sources_content_block());

  const auto& web_sources = (*result)->get_web_sources_content_block();
  // Only the valid source should be included
  ASSERT_EQ(web_sources->sources.size(), 1u);
  EXPECT_EQ(web_sources->sources[0]->title, "Valid Source");
}

TEST(OaiParsingTest, ParseContentBlockFromDict_WebSourcesEmptySources) {
  constexpr char kBlockJson[] = R"({
    "type": "brave-chat.webSources",
    "sources": []
  })";

  auto block = base::test::ParseJsonDict(kBlockJson);
  auto result = ParseContentBlockFromDict(block);

  // Empty sources and no query should return nullopt
  EXPECT_FALSE(result.has_value());
}

TEST(OaiParsingTest, ParseContentBlockFromDict_WebSourcesQueryOnly) {
  constexpr char kBlockJson[] = R"({
    "type": "brave-chat.webSources",
    "sources": [],
    "query": "search term"
  })";

  auto block = base::test::ParseJsonDict(kBlockJson);
  auto result = ParseContentBlockFromDict(block);

  // Query without sources should still be valid
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE((*result)->is_web_sources_content_block());
  EXPECT_EQ((*result)->get_web_sources_content_block()->query, "search term");
  EXPECT_TRUE((*result)->get_web_sources_content_block()->sources.empty());
}

TEST(OaiParsingTest,
     ParseContentBlockFromDict_WebSourcesMissingRequiredFields) {
  // Sources missing title or url should be skipped
  constexpr char kBlockJson[] = R"({
    "type": "brave-chat.webSources",
    "sources": [
      {
        "url": "https://notitle.com"
      },
      {
        "title": "No URL"
      },
      {
        "title": "Valid",
        "url": "https://valid.com"
      }
    ]
  })";

  auto block = base::test::ParseJsonDict(kBlockJson);
  auto result = ParseContentBlockFromDict(block);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE((*result)->is_web_sources_content_block());

  const auto& web_sources = (*result)->get_web_sources_content_block();
  ASSERT_EQ(web_sources->sources.size(), 1u);
  EXPECT_EQ(web_sources->sources[0]->title, "Valid");
}

TEST(OaiParsingTest,
     ParseContentBlockFromDict_WebSourcesSkipsNonDictSourceItems) {
  // Non-dict items in sources list should be skipped
  constexpr char kBlockJson[] = R"({
    "type": "brave-chat.webSources",
    "sources": [
      "invalid string",
      123,
      {
        "title": "Valid Source",
        "url": "https://valid.com"
      },
      null
    ]
  })";

  auto block = base::test::ParseJsonDict(kBlockJson);
  auto result = ParseContentBlockFromDict(block);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE((*result)->is_web_sources_content_block());

  const auto& web_sources = (*result)->get_web_sources_content_block();
  ASSERT_EQ(web_sources->sources.size(), 1u);
  EXPECT_EQ(web_sources->sources[0]->title, "Valid Source");
}

TEST(OaiParsingTest, ParseContentBlockFromDict_WebSourcesInvalidUrl) {
  constexpr char kBlockJson[] = R"({
    "type": "brave-chat.webSources",
    "sources": [
      {
        "title": "Invalid URL",
        "url": "not-a-valid-url"
      },
      {
        "title": "Valid",
        "url": "https://valid.com"
      }
    ]
  })";

  auto block = base::test::ParseJsonDict(kBlockJson);
  auto result = ParseContentBlockFromDict(block);

  ASSERT_TRUE(result.has_value());
  const auto& web_sources = (*result)->get_web_sources_content_block();
  ASSERT_EQ(web_sources->sources.size(), 1u);
  EXPECT_EQ(web_sources->sources[0]->title, "Valid");
}

TEST(OaiParsingTest, ParseContentBlockFromDict_MissingType) {
  constexpr char kBlockJson[] = R"({
    "text": "No type field"
  })";

  auto block = base::test::ParseJsonDict(kBlockJson);
  auto result = ParseContentBlockFromDict(block);

  EXPECT_FALSE(result.has_value());
}

TEST(OaiParsingTest, ParseContentBlockFromDict_UnsupportedType) {
  constexpr char kBlockJson[] = R"({
    "type": "unknown_type",
    "data": "some data"
  })";

  auto block = base::test::ParseJsonDict(kBlockJson);
  auto result = ParseContentBlockFromDict(block);

  EXPECT_FALSE(result.has_value());
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
  base::DictValue properties;
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
  base::DictValue properties;
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
  base::DictValue extra_params;
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
    base::DictValue extra_params;
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
  base::DictValue extra_params;
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

  base::DictValue extra_params;
  extra_params.Set("screen_width", 1920);
  auto custom_tool = std::make_unique<MockTool>(
      "screen_tool", "", "computer_20241022", std::nullopt, std::nullopt,
      std::move(extra_params));

  base::DictValue properties;
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

// Tests for ParseOAICompletionResponse

TEST(OAIParsingTest, ParseOAICompletionResponse_ValidStreamingResponse) {
  // Test parsing a valid streaming response (delta.content)
  constexpr char kResponseJson[] = R"({
    "model": "gpt-3.5-turbo",
    "choices": [{
      "delta": {
        "content": "Hello, world!"
      }
    }]
  })";

  auto response_dict = base::test::ParseJsonDict(kResponseJson);
  auto result = ParseOAICompletionResponse(response_dict, "model_key");

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_completion_event());
  EXPECT_EQ(result->event->get_completion_event()->completion, "Hello, world!");
  EXPECT_EQ(result->model_key, "model_key");
}

TEST(OAIParsingTest, ParseOAICompletionResponse_ValidNonStreamingResponse) {
  // Test parsing a valid non-streaming response (message.content)
  constexpr char kResponseJson[] = R"({
    "model": "gpt-3.5-turbo",
    "choices": [{
      "message": {
        "content": "Test response"
      }
    }]
  })";

  auto response_dict = base::test::ParseJsonDict(kResponseJson);
  auto result = ParseOAICompletionResponse(response_dict, std::nullopt);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->event);
  ASSERT_TRUE(result->event->is_completion_event());
  EXPECT_EQ(result->event->get_completion_event()->completion, "Test response");
  EXPECT_FALSE(result->model_key.has_value());
}

TEST(OAIParsingTest, ParseOAICompletionResponse_EmptyContent) {
  // Test with empty content string (should return nullopt)
  constexpr char kResponseJson[] = R"({
    "model": "gpt-3.5-turbo",
    "choices": [{
      "delta": {
        "content": ""
      }
    }]
  })";

  auto response_dict = base::test::ParseJsonDict(kResponseJson);
  auto result = ParseOAICompletionResponse(response_dict, std::nullopt);

  ASSERT_FALSE(result.has_value());
}

struct InvalidResponseTestCase {
  const char* test_name;
  const char* response_json;
};

class ParseOAICompletionResponseInvalidTest
    : public ::testing::TestWithParam<InvalidResponseTestCase> {};

TEST_P(ParseOAICompletionResponseInvalidTest, ReturnsNullopt) {
  const InvalidResponseTestCase& test_case = GetParam();
  auto response_dict = base::test::ParseJsonDict(test_case.response_json);
  auto result = ParseOAICompletionResponse(response_dict, std::nullopt);
  EXPECT_FALSE(result.has_value());
}

INSTANTIATE_TEST_SUITE_P(
    ,
    ParseOAICompletionResponseInvalidTest,
    ::testing::Values(
        InvalidResponseTestCase{"MissingChoices",
                                R"({"model": "gpt-3.5-turbo"})"},
        InvalidResponseTestCase{"EmptyChoicesArray",
                                R"({"model": "gpt-3.5-turbo", "choices": []})"},
        InvalidResponseTestCase{
            "NonDictChoice",
            R"({"model": "gpt-3.5-turbo", "choices": ["invalid"]})"},
        InvalidResponseTestCase{
            "NoDeltaOrMessage",
            R"({"model": "gpt-3.5-turbo", "choices": [{"index": 0}]})"},
        InvalidResponseTestCase{"NoContentInDelta",
                                R"({"model": "gpt-3.5-turbo",
                                    "choices": [
                                      {"delta": {"role": "assistant"}}
                                    ]})"}),
    [](const ::testing::TestParamInfo<InvalidResponseTestCase>& info) {
      return info.param.test_name;
    });

}  // namespace ai_chat
