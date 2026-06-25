/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/engine/oai_serialization_utils.h"

#include <string>

#include "brave/components/ai_chat/core/browser/engine/oai_message_utils.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

// Serializes a single tool call with the given `arguments_json` and returns the
// resulting "arguments" string written to the message dict.
std::string SerializeToolCallArguments(const std::string& arguments_json) {
  OAIMessage message;
  message.tool_calls.push_back(
      mojom::ToolUseEvent::New("tool", "call_1", arguments_json, std::nullopt,
                               std::nullopt, nullptr, false));

  base::DictValue dict;
  SerializeToolCallsOnMessageDict(message, dict);

  const base::ListValue* tool_calls = dict.FindList("tool_calls");
  EXPECT_TRUE(tool_calls);
  EXPECT_EQ(tool_calls->size(), 1u);
  const base::DictValue* function =
      (*tool_calls)[0].GetDict().FindDict("function");
  EXPECT_TRUE(function);
  return *function->FindString("arguments");
}

}  // namespace

TEST(OAISerializationUtilsTest, MemoryContentBlockToDict_StringValues) {
  auto block = mojom::MemoryContentBlock::New();
  block->memory["name"] = mojom::MemoryValue::NewStringValue("Alice");
  block->memory["city"] = mojom::MemoryValue::NewStringValue("NYC");

  auto dict = MemoryContentBlockToDict(*block);

  const std::string* name = dict.FindString("name");
  ASSERT_TRUE(name);
  EXPECT_EQ(*name, "Alice");

  const std::string* city = dict.FindString("city");
  ASSERT_TRUE(city);
  EXPECT_EQ(*city, "NYC");
}

TEST(OAISerializationUtilsTest, MemoryContentBlockToDict_ListValues) {
  auto block = mojom::MemoryContentBlock::New();
  block->memory["hobbies"] =
      mojom::MemoryValue::NewListValue({"reading", "coding"});

  auto dict = MemoryContentBlockToDict(*block);

  const base::ListValue* hobbies = dict.FindList("hobbies");
  ASSERT_TRUE(hobbies);
  ASSERT_EQ(hobbies->size(), 2u);
  EXPECT_EQ((*hobbies)[0].GetString(), "reading");
  EXPECT_EQ((*hobbies)[1].GetString(), "coding");
}

TEST(OAISerializationUtilsTest, MemoryContentBlockToDict_MixedValues) {
  auto block = mojom::MemoryContentBlock::New();
  block->memory["name"] = mojom::MemoryValue::NewStringValue("Bob");
  block->memory["langs"] = mojom::MemoryValue::NewListValue({"C++", "Python"});

  auto dict = MemoryContentBlockToDict(*block);

  const std::string* name = dict.FindString("name");
  ASSERT_TRUE(name);
  EXPECT_EQ(*name, "Bob");

  const base::ListValue* langs = dict.FindList("langs");
  ASSERT_TRUE(langs);
  ASSERT_EQ(langs->size(), 2u);
  EXPECT_EQ((*langs)[0].GetString(), "C++");
  EXPECT_EQ((*langs)[1].GetString(), "Python");
}

TEST(OAISerializationUtilsTest, MemoryContentBlockToDict_Empty) {
  auto block = mojom::MemoryContentBlock::New();
  auto dict = MemoryContentBlockToDict(*block);
  EXPECT_TRUE(dict.empty());
}

TEST(OAISerializationUtilsTest, FileContentBlockToDict) {
  auto block = mojom::FileContentBlock::New();
  block->filename = "test.pdf";
  block->file_data = GURL("data:application/pdf;base64,abc123");

  auto dict = FileContentBlockToDict(*block);

  const std::string* filename = dict.FindString("filename");
  ASSERT_TRUE(filename);
  EXPECT_EQ(*filename, "test.pdf");

  const std::string* file_data = dict.FindString("file_data");
  ASSERT_TRUE(file_data);
  EXPECT_EQ(*file_data, "data:application/pdf;base64,abc123");
}

TEST(OAISerializationUtilsTest, ImageContentBlockToDict) {
  auto block = mojom::ImageContentBlock::New();
  block->image_url = GURL("data:image/png;base64,xyz789");

  auto dict = ImageContentBlockToDict(*block);

  const std::string* url = dict.FindString("url");
  ASSERT_TRUE(url);
  EXPECT_EQ(*url, "data:image/png;base64,xyz789");
}

TEST(OAISerializationUtilsTest, SerializeToolCallsOnMessageDict_Empty) {
  OAIMessage message;
  // No tool calls, empty tool_call_id
  base::DictValue dict;
  dict.Set("role", "assistant");
  SerializeToolCallsOnMessageDict(message, dict);

  // Dict should be unchanged - no tool_calls or tool_call_id added
  EXPECT_FALSE(dict.FindList("tool_calls"));
  EXPECT_FALSE(dict.FindString("tool_call_id"));
  // Original key still present
  EXPECT_TRUE(dict.FindString("role"));
}

TEST(OAISerializationUtilsTest,
     SerializeToolCallsOnMessageDict_WithToolCallsAndToolCallId) {
  OAIMessage message;
  message.tool_calls.push_back(mojom::ToolUseEvent::New(
      "brave_web_search", "call_1", R"({"query":"weather"})", std::nullopt,
      std::nullopt, nullptr, false));
  message.tool_calls.push_back(
      mojom::ToolUseEvent::New("get_time", "call_2", R"({"tz":"UTC"})",
                               std::nullopt, std::nullopt, nullptr, false));
  message.tool_call_id = "call_0";

  base::DictValue dict;
  dict.Set("role", "tool");
  SerializeToolCallsOnMessageDict(message, dict);

  // Verify tool_calls list
  const base::ListValue* tool_calls = dict.FindList("tool_calls");
  ASSERT_TRUE(tool_calls);
  ASSERT_EQ(tool_calls->size(), 2u);

  // First tool call
  const base::DictValue* tc0 = (*tool_calls)[0].GetIfDict();
  ASSERT_TRUE(tc0);
  EXPECT_EQ(*tc0->FindString("id"), "call_1");
  EXPECT_EQ(*tc0->FindString("type"), "function");
  const base::DictValue* fn0 = tc0->FindDict("function");
  ASSERT_TRUE(fn0);
  EXPECT_EQ(*fn0->FindString("name"), "brave_web_search");
  EXPECT_EQ(*fn0->FindString("arguments"), R"({"query":"weather"})");

  // Second tool call
  const base::DictValue* tc1 = (*tool_calls)[1].GetIfDict();
  ASSERT_TRUE(tc1);
  EXPECT_EQ(*tc1->FindString("id"), "call_2");
  EXPECT_EQ(*tc1->FindString("type"), "function");
  const base::DictValue* fn1 = tc1->FindDict("function");
  ASSERT_TRUE(fn1);
  EXPECT_EQ(*fn1->FindString("name"), "get_time");
  EXPECT_EQ(*fn1->FindString("arguments"), R"({"tz":"UTC"})");

  // Verify tool_call_id
  const std::string* tool_call_id = dict.FindString("tool_call_id");
  ASSERT_TRUE(tool_call_id);
  EXPECT_EQ(*tool_call_id, "call_0");
}

TEST(OAISerializationUtilsTest,
     SerializeToolCallsOnMessageDict_EmptyArgumentsNormalizedToObject) {
  // Some models emit an empty string for the arguments of a tool that takes no
  // parameters. An empty string is not valid JSON, so it must be normalized to
  // an empty object before being echoed back to the server.
  EXPECT_EQ(SerializeToolCallArguments(""), "{}");
}

TEST(OAISerializationUtilsTest,
     SerializeToolCallsOnMessageDict_NonEmptyArgumentsUnchanged) {
  // Non-empty arguments must be passed through verbatim.
  EXPECT_EQ(SerializeToolCallArguments(R"({"query":"weather"})"),
            R"({"query":"weather"})");
}

TEST(OAISerializationUtilsTest,
     SerializeToolCallsOnMessageDict_EmptyIshJsonValuesUnchanged) {
  // Only a truly empty string is normalized. Valid JSON values that merely look
  // "empty" (null, an empty JSON string, an empty array, an empty object) are
  // non-empty strings and must be passed through verbatim.
  for (const char* args : {"null", R"("")", "[]", "{}"}) {
    EXPECT_EQ(SerializeToolCallArguments(args), args) << "args=" << args;
  }
}

}  // namespace ai_chat
