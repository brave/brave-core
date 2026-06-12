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

#include "base/json/json_reader.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/test/base/testing_profile.h"
#include "components/history_embeddings/core/history_embeddings_search.h"
#include "components/history_embeddings/core/vector_database.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"
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
  ASSERT_TRUE(challenge->plan.has_value());
  EXPECT_THAT(*challenge->plan,
              testing::HasSubstr("semantically search your currently-open"));
  EXPECT_THAT(*challenge->plan, testing::HasSubstr("stay on this device"));
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
  // TabTrackerService is not registered for the TestingProfile so
  // SnapshotOpenTabs() returns empty, exercising the early-return JSON path.
  std::string output = RunTool(tool_.get(), kValidInput);
  EXPECT_THAT(output, testing::HasSubstr("\"results\":[]"));
}

namespace {

// Builds a single ScoredUrlRow with a one-passage UrlData so
// `GetBestPassage()` returns the supplied passage text.
history_embeddings::ScoredUrlRow MakeScoredRow(history::URLID url_id,
                                               float score,
                                               const std::string& passage) {
  history_embeddings::ScoredUrl scored(url_id, /*visit_id=*/0, base::Time(),
                                       score,
                                       /*word_match_score=*/0.0f);
  history_embeddings::ScoredUrlRow row(std::move(scored));
  row.url_data =
      history_embeddings::UrlData(url_id, /*visit_id=*/0, base::Time());
  row.url_data.passages.add_passages(passage);
  row.url_data.passage_embeddings.emplace_back(
      history_embeddings::PassageEmbedding{
          passage_embeddings::Embedding(std::vector<float>(1, 1.0f)),
          /*word_count=*/1});
  row.scores.push_back(score);
  return row;
}

}  // namespace

TEST(TabSemanticSearchToolBuildResultsJsonTest, MapsScoredRowsToTabsByUrlId) {
  // Open tabs: distinct URLIDs.
  std::vector<internal::SemanticSearchTabInfo> tabs;
  tabs.push_back({/*tab_id=*/11, "Tab One", GURL("https://one.example/"),
                  /*url_id=*/100});
  tabs.push_back({/*tab_id=*/22, "Tab Two", GURL("https://two.example/"),
                  /*url_id=*/200});
  tabs.push_back({/*tab_id=*/33, "Untracked", GURL("https://three.example/"),
                  /*url_id=*/0});

  history_embeddings::SearchResult result;
  // Note: `row.row.url()` is intentionally a *different* GURL than the tab's
  // (history's canonical form). The mapping must be by `url_id`, not URL.
  result.scored_url_rows.push_back(
      MakeScoredRow(/*url_id=*/200, /*score=*/0.9f, "two-snippet"));
  result.scored_url_rows.back().row.set_url(
      GURL("https://two.example/?utm_source=history"));
  result.scored_url_rows.push_back(
      MakeScoredRow(/*url_id=*/100, /*score=*/0.7f, "one-snippet"));
  // A URLID not in any open tab — must be dropped.
  result.scored_url_rows.push_back(
      MakeScoredRow(/*url_id=*/999, /*score=*/0.6f, "ghost-snippet"));

  std::string json =
      internal::BuildSemanticSearchResultsJson(tabs, result, /*count=*/5);
  auto parsed = base::JSONReader::ReadDict(json, base::JSON_PARSE_RFC);
  ASSERT_TRUE(parsed.has_value());
  const auto* results = parsed->FindList("results");
  ASSERT_TRUE(results);
  ASSERT_EQ(results->size(), 2u);

  // First emitted result is the higher-scoring row (URLID 200 → tab 22).
  const auto& first = (*results)[0].GetDict();
  EXPECT_EQ(*first.FindString("tab_id"), "22");
  EXPECT_EQ(*first.FindString("title"), "Tab Two");
  EXPECT_EQ(*first.FindString("url"), "https://two.example/");

  // Second: URLID 100 → tab 11.
  const auto& second = (*results)[1].GetDict();
  EXPECT_EQ(*second.FindString("tab_id"), "11");
  EXPECT_EQ(*second.FindString("title"), "Tab One");
}

TEST(TabSemanticSearchToolBuildResultsJsonTest, RespectsCountCap) {
  std::vector<internal::SemanticSearchTabInfo> tabs;
  for (int i = 0; i < 5; ++i) {
    tabs.push_back({/*tab_id=*/i, "Tab", GURL("https://example.test/"),
                    /*url_id=*/100 + i});
  }
  history_embeddings::SearchResult result;
  for (int i = 0; i < 5; ++i) {
    result.scored_url_rows.push_back(
        MakeScoredRow(/*url_id=*/100 + i, /*score=*/1.0f - i * 0.1f, "s"));
  }

  std::string json =
      internal::BuildSemanticSearchResultsJson(tabs, result, /*count=*/2);
  auto parsed = base::JSONReader::ReadDict(json, base::JSON_PARSE_RFC);
  ASSERT_TRUE(parsed.has_value());
  ASSERT_EQ(parsed->FindList("results")->size(), 2u);
}

TEST(TabSemanticSearchToolBuildResultsJsonTest, SkipsTabsWithoutUrlId) {
  std::vector<internal::SemanticSearchTabInfo> tabs;
  // A tab whose URLID hasn't been resolved yet — it shouldn't be matched even
  // if a Search row arrives with url_id 0 (which would be a bug elsewhere,
  // but the mapping should still defend against it).
  tabs.push_back({/*tab_id=*/7, "Pending", GURL("https://pending.example/"),
                  /*url_id=*/0});

  history_embeddings::SearchResult result;
  result.scored_url_rows.push_back(
      MakeScoredRow(/*url_id=*/0, /*score=*/1.0f, "p"));

  std::string json =
      internal::BuildSemanticSearchResultsJson(tabs, result, /*count=*/5);
  EXPECT_THAT(json, testing::HasSubstr("\"results\":[]"));
}

}  // namespace ai_chat
