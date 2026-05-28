// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/tab_semantic_search_tool.h"

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "brave/browser/history_embeddings/open_tab_search.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr char kValidInput[] = R"({"query":"react hooks"})";

std::string ExtractText(const std::vector<mojom::ContentBlockPtr>& blocks) {
  if (blocks.empty() || !blocks[0]->is_text_content_block()) {
    return std::string();
  }
  return blocks[0]->get_text_content_block()->text;
}

std::string RunTool(TabSemanticSearchTool* tool, const std::string& json) {
  base::test::TestFuture<std::vector<mojom::ContentBlockPtr>,
                         std::vector<mojom::ToolArtifactPtr>>
      future;
  tool->UseTool(json, future.GetCallback());
  return ExtractText(future.Get<std::vector<mojom::ContentBlockPtr>>());
}

mojom::ToolUseEventPtr CreateToolUseEvent(const std::string& json) {
  return mojom::ToolUseEvent::New(mojom::kSemanticTabSearchToolName, "1", json,
                                  std::nullopt, std::nullopt, nullptr, false);
}

}  // namespace

class TabSemanticSearchToolTest : public ::testing::Test {
 public:
  TabSemanticSearchToolTest() = default;
  ~TabSemanticSearchToolTest() override = default;

  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    tool_ = std::make_unique<TabSemanticSearchTool>(profile_.get());
  }

  void TearDown() override {
    tool_.reset();
    profile_.reset();
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<TabSemanticSearchTool> tool_;
};

TEST_F(TabSemanticSearchToolTest, NameMatchesMojomConstant) {
  EXPECT_EQ(tool_->Name(), mojom::kSemanticTabSearchToolName);
}

TEST_F(TabSemanticSearchToolTest, RequiresPermissionChallengeFirstTime) {
  auto event = CreateToolUseEvent(kValidInput);
  auto result = tool_->RequiresUserInteractionBeforeHandling(*event);
  ASSERT_TRUE(std::holds_alternative<mojom::PermissionChallengePtr>(result));
  const auto& challenge = std::get<mojom::PermissionChallengePtr>(result);
  ASSERT_TRUE(challenge);
  // The C++ side surfaces a non-null challenge; user-facing copy is provided
  // by the untrusted frame via `getToolPermissionImplications`.
  EXPECT_FALSE(challenge->assessment.has_value());
  EXPECT_FALSE(challenge->plan.has_value());
}

TEST_F(TabSemanticSearchToolTest, NoChallengeAfterPermissionGranted) {
  tool_->UserPermissionGranted("1");
  auto event = CreateToolUseEvent(kValidInput);
  auto result = tool_->RequiresUserInteractionBeforeHandling(*event);
  ASSERT_TRUE(std::holds_alternative<bool>(result));
  EXPECT_FALSE(std::get<bool>(result));
}

TEST_F(TabSemanticSearchToolTest, UseToolWithoutPermissionFails) {
  EXPECT_THAT(RunTool(tool_.get(), kValidInput),
              testing::HasSubstr("Permission to search open tabs"));
}

TEST_F(TabSemanticSearchToolTest, UseToolWithInvalidJsonFails) {
  tool_->UserPermissionGranted("1");
  EXPECT_THAT(RunTool(tool_.get(), "[]"),
              testing::HasSubstr("Failed to parse input JSON"));
}

TEST_F(TabSemanticSearchToolTest, UseToolMissingQueryFails) {
  tool_->UserPermissionGranted("1");
  EXPECT_THAT(RunTool(tool_.get(), "{}"),
              testing::HasSubstr("Missing required 'query' field"));
}

TEST_F(TabSemanticSearchToolTest, UseToolEmptyQueryFails) {
  tool_->UserPermissionGranted("1");
  EXPECT_THAT(RunTool(tool_.get(), R"({"query":""})"),
              testing::HasSubstr("Missing required 'query' field"));
}

TEST_F(TabSemanticSearchToolTest, NoOpenTabsReturnsEmptyResults) {
  tool_->UserPermissionGranted("1");
  // `SearchOpenTabsByContent()` returns no tabs for a `TestingProfile` with
  // no browser windows, exercising the early-return JSON path.
  std::string output = RunTool(tool_.get(), kValidInput);
  EXPECT_EQ(base::test::ParseJsonDict(output),
            base::test::ParseJsonDict(R"({"results":[]})"));
}

TEST(TabSemanticSearchToolBuildResultsJsonTest, EmitsRankedTabs) {
  const std::vector<history_embeddings::OpenTabInfo> tabs = {
      {/*tab_id=*/22, "Tab Two", GURL("https://two.example/")},
      {/*tab_id=*/11, "Tab One", GURL("https://one.example/")},
  };

  EXPECT_EQ(
      base::test::ParseJsonDict(internal::BuildSemanticSearchResultsJson(tabs)),
      base::test::ParseJsonDict(R"({
        "results": [
          {"tab_id": 22, "title": "Tab Two", "url": "https://two.example/"},
          {"tab_id": 11, "title": "Tab One", "url": "https://one.example/"}
        ]
      })"));
}

TEST(TabSemanticSearchToolBuildResultsJsonTest, EmptyTabsEmitsEmptyResults) {
  EXPECT_EQ(
      base::test::ParseJsonDict(internal::BuildSemanticSearchResultsJson({})),
      base::test::ParseJsonDict(R"({"results":[]})"));
}

TEST(TabSemanticSearchToolBuildTabSourcesJsonTest, EmitsRankedTabSources) {
  const std::vector<history_embeddings::OpenTabInfo> tabs = {
      {/*tab_id=*/22, "Tab Two", GURL("https://two.example/")},
      {/*tab_id=*/11, "Tab One", GURL("https://one.example/")},
  };

  // tab_id is emitted as an int32 so the frontend can hand it straight to
  // `SwitchToTab(int32)` without re-parsing.
  EXPECT_EQ(base::test::ParseJsonDict(
                internal::BuildSemanticSearchTabSourcesJson(tabs)),
            base::test::ParseJsonDict(R"({
              "sources": [
                {"tab_id": 22, "title": "Tab Two",
                 "url": "https://two.example/"},
                {"tab_id": 11, "title": "Tab One",
                 "url": "https://one.example/"}
              ]
            })"));
}

TEST(TabSemanticSearchToolBuildTabSourcesJsonTest, EmptyTabsEmitsEmptyString) {
  // Empty payload signals `OnRanked` to skip emitting a tab_sources artifact.
  EXPECT_TRUE(internal::BuildSemanticSearchTabSourcesJson({}).empty());
}

}  // namespace ai_chat
