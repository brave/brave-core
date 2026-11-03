/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/mcp/mcp_types.h"

#include <string>
#include <variant>

#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJsonDict;

namespace ai_chat {

TEST(MCPTypesTest, ParseToolDefinition) {
  auto tool = ParseToolDefinition(ParseJsonDict(R"({
    "name": "search",
    "title": "Search",
    "description": "Searches",
    "inputSchema": {"type": "object", "properties": {"q": {"type": "string"}}},
    "icons": [{"src": "https://example.com/icon.png"}],
    "annotations": {"readOnlyHint": true}
  })"));
  ASSERT_TRUE(tool);
  EXPECT_EQ(tool->name, "search");
  EXPECT_EQ(tool->title, "Search");
  EXPECT_EQ(tool->description, "Searches");
  EXPECT_EQ(tool->input_schema,
            ParseJsonDict(
                R"({"type":"object","properties":{"q":{"type":"string"}}})"));

  tool = ParseToolDefinition(
      ParseJsonDict(R"({"name":"a","inputSchema":{"type":"object"}})"));
  ASSERT_TRUE(tool);
  EXPECT_FALSE(tool->title);
  EXPECT_FALSE(tool->description);

  const MCPToolDefinition clone = tool->Clone();
  EXPECT_EQ(clone.name, tool->name);
  EXPECT_EQ(clone.input_schema, tool->input_schema);
}

TEST(MCPTypesTest, ParseToolDefinitionRejectsMalformed) {
  const std::string long_name(129, 'a');
  for (const std::string& json : {
           std::string(R"({"inputSchema":{}})"),
           std::string(R"({"name":"","inputSchema":{}})"),
           std::string(R"({"name":5,"inputSchema":{}})"),
           std::string(R"({"name":"a\nb","inputSchema":{}})"),
           R"({"name":")" + long_name + R"(","inputSchema":{}})",
           std::string(R"({"name":"a"})"),
           std::string(R"({"name":"a","inputSchema":"object"})"),
           std::string(R"({"name":"a","inputSchema":{},"description":1})"),
           std::string(R"({"name":"a","inputSchema":{},"title":[]})"),
       }) {
    SCOPED_TRACE(json);
    EXPECT_FALSE(ParseToolDefinition(ParseJsonDict(json)));
  }
  // The longest allowed name.
  EXPECT_TRUE(ParseToolDefinition(ParseJsonDict(
      R"({"name":")" + std::string(128, 'a') + R"(","inputSchema":{}})")));
}

TEST(MCPTypesTest, ParseCallToolResult) {
  auto result = ParseCallToolResult(ParseJsonDict(R"({
    "content": [
      {"type": "text", "text": "hello"},
      {"type": "image", "data": "aGVsbG8=", "mimeType": "image/png"},
      {"type": "audio", "data": "aGVsbG8=", "mimeType": "audio/wav"},
      {"type": "resource_link", "uri": "file:///etc/passwd"},
      {"type": "resource", "resource": {"uri": "a", "text": "b"}},
      {"type": "image", "data": "aGVsbG8=", "mimeType": "image/svg+xml"},
      {"type": "image", "data": "not base64!", "mimeType": "image/png"},
      {"type": "text"},
      {"text": "no type"},
      "string"
    ],
    "structuredContent": {"a": 1},
    "isError": false,
    "_meta": {}
  })"));
  ASSERT_TRUE(result);
  ASSERT_EQ(result->content.size(), 2u);
  EXPECT_EQ(std::get<MCPTextContent>(result->content[0]).text, "hello");
  EXPECT_EQ(std::get<MCPImageContent>(result->content[1]),
            (MCPImageContent{"aGVsbG8=", "image/png"}));
  EXPECT_EQ(result->structured_content, ParseJsonDict(R"({"a":1})"));
  EXPECT_FALSE(result->is_error);

  result = ParseCallToolResult(ParseJsonDict(
      R"({"content":[{"type":"text","text":"failed"}],"isError":true})"));
  ASSERT_TRUE(result);
  EXPECT_TRUE(result->is_error);
  EXPECT_FALSE(result->structured_content);

  result = ParseCallToolResult(base::DictValue());
  ASSERT_TRUE(result);
  EXPECT_TRUE(result->content.empty());
}

TEST(MCPTypesTest, ParseCallToolResultRejectsMalformed) {
  for (const char* json : {
           R"({"content":"text"})",
           R"({"content":[],"isError":"true"})",
           R"({"content":[],"structuredContent":[1]})",
       }) {
    SCOPED_TRACE(json);
    EXPECT_FALSE(ParseCallToolResult(ParseJsonDict(json)));
  }
}

}  // namespace ai_chat
