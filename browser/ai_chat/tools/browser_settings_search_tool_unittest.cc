// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/browser_settings_search_tool.h"

#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

std::string ExtractText(const std::vector<mojom::ContentBlockPtr>& blocks) {
  if (blocks.empty() || !blocks[0]->is_text_content_block()) {
    return std::string();
  }
  return blocks[0]->get_text_content_block()->text;
}

mojom::ToolUseEventPtr MakeToolUseEvent(const std::string& arguments_json) {
  auto event = mojom::ToolUseEvent::New();
  event->tool_name = mojom::kBrowserSettingsSearchToolName;
  event->id = "tool-use-id";
  event->arguments_json = arguments_json;
  return event;
}

}  // namespace

class BrowserSettingsSearchToolTest : public testing::Test {
 protected:
  // Runs the tool and returns its output parsed as a dict. On error output
  // the dict is empty; `text_output_` holds the raw output either way.
  base::DictValue RunTool(const std::string& json) {
    base::test::TestFuture<std::vector<mojom::ContentBlockPtr>,
                           std::vector<mojom::ToolArtifactPtr>>
        future;
    tool_.UseTool(json, future.GetCallback());
    text_output_ =
        ExtractText(future.Get<std::vector<mojom::ContentBlockPtr>>());
    auto dict = base::JSONReader::ReadDict(text_output_, base::JSON_PARSE_RFC);
    return dict ? std::move(*dict) : base::DictValue();
  }

  BrowserSettingsSearchTool tool_;
  std::string text_output_;
};

// Results are a flat list of preference paths -- there is no per-setting
// metadata to return.
TEST_F(BrowserSettingsSearchToolTest, ReturnsMatchingPreferencePaths) {
  auto result = RunTool(R"({"query": "clear cookies when I quit"})");
  EXPECT_EQ(*result.FindString("query"), "clear cookies when I quit");

  const base::ListValue* results = result.FindList("results");
  ASSERT_TRUE(results);
  ASSERT_FALSE(results->empty());

  std::vector<std::string> paths;
  for (const auto& value : *results) {
    ASSERT_TRUE(value.is_string());
    paths.push_back(value.GetString());
  }
  EXPECT_THAT(paths, testing::Contains("browser.clear_data.cookies_on_exit"));
}

// The whole point of splitting search from value lookup is that search never
// reveals anything about the user. Plain strings can't carry a value at all,
// which is a structural guarantee rather than one we have to remember.
TEST_F(BrowserSettingsSearchToolTest, NeverReturnsValues) {
  auto result = RunTool(R"({"query": "ai chat memory"})");
  const base::ListValue* results = result.FindList("results");
  ASSERT_TRUE(results);
  ASSERT_FALSE(results->empty());
  for (const auto& value : *results) {
    EXPECT_TRUE(value.is_string());
  }
}

TEST_F(BrowserSettingsSearchToolTest, RespectsCountAndCapsIt) {
  auto result = RunTool(R"({"query": "clear data on exit", "count": 2})");
  EXPECT_LE(result.FindList("results")->size(), 2u);

  // Out-of-range counts are clamped rather than rejected, since the model
  // routinely picks arbitrary numbers.
  result = RunTool(R"({"query": "clear data on exit", "count": 9999})");
  EXPECT_LE(result.FindList("results")->size(), 40u);

  result = RunTool(R"({"query": "clear data on exit", "count": -5})");
  EXPECT_GE(result.FindList("results")->size(), 1u);
}

TEST_F(BrowserSettingsSearchToolTest, ReturnsEmptyResultsForUnknownQuery) {
  auto result = RunTool(R"({"query": "zzzzqqqq gibberish"})");
  EXPECT_TRUE(result.FindList("results")->empty());
}

TEST_F(BrowserSettingsSearchToolTest, RejectsMalformedInput) {
  for (const char* input :
       {"not json", "{}", R"({"query": ""})", R"({"query": 42})"}) {
    RunTool(input);
    EXPECT_THAT(text_output_, testing::StartsWith("Error:")) << input;
  }
}

TEST_F(BrowserSettingsSearchToolTest, AsksForPermissionOnlyUntilGranted) {
  auto event = MakeToolUseEvent(R"({"query": "ai chat"})");
  auto first = tool_.RequiresUserInteractionBeforeHandling(*event);
  ASSERT_TRUE(std::holds_alternative<mojom::PermissionChallengePtr>(first));
  EXPECT_TRUE(std::get<mojom::PermissionChallengePtr>(first));

  tool_.UserPermissionGranted("tool-use-id");

  auto second = tool_.RequiresUserInteractionBeforeHandling(*event);
  ASSERT_TRUE(std::holds_alternative<bool>(second));
  EXPECT_FALSE(std::get<bool>(second));
}

}  // namespace ai_chat
