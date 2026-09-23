// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/history_search_tool.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/test/base/testing_profile.h"
#include "components/history/core/browser/url_row.h"
#include "components/history_embeddings/content/history_embeddings_service.h"
#include "components/history_embeddings/core/history_embeddings_features.h"
#include "components/history_embeddings/core/history_embeddings_search.h"
#include "components/history_embeddings/core/vector_database.h"
#include "components/passage_embeddings/core/passage_embeddings_types.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

std::string ExtractText(const std::vector<mojom::ContentBlockPtr>& blocks) {
  if (blocks.empty() || !blocks[0]->is_text_content_block()) {
    return std::string();
  }
  return blocks[0]->get_text_content_block()->text;
}

std::string RunTool(HistorySearchTool* tool, const std::string& json) {
  base::test::TestFuture<std::vector<mojom::ContentBlockPtr>,
                         std::vector<mojom::ToolArtifactPtr>>
      future;
  tool->UseTool(json, future.GetCallback());
  return ExtractText(future.Get<std::vector<mojom::ContentBlockPtr>>());
}

history_embeddings::ScoredUrlRow MakeRow(const std::string& url,
                                         const std::u16string& title) {
  history_embeddings::ScoredUrl scored_url(/*url_id=*/1, /*visit_id=*/1,
                                           base::Time::Now(),
                                           /*score=*/0.9f,
                                           /*word_match_score=*/0.0f);
  history_embeddings::ScoredUrlRow row(scored_url);
  row.row.set_url(GURL(url));
  row.row.set_title(title);
  row.row.set_last_visit(base::Time::FromTimeT(1700000000));
  return row;
}

// Attaches a passage + matching embedding/score to a ScoredUrlRow. The
// embedding's data is irrelevant for serialization; only the score and word
// count are read by GetBestScoreIndices.
void AddPassage(history_embeddings::ScoredUrlRow& row,
                const std::string& passage,
                float score,
                size_t word_count) {
  row.url_data.passages.add_passages(passage);
  // Embedding's ctor DCHECKs that the vector has unit magnitude; {1.0f} does.
  row.url_data.passage_embeddings.emplace_back(
      history_embeddings::PassageEmbedding{
          passage_embeddings::Embedding(std::vector<float>{1.0f}), word_count});
  row.scores.push_back(score);
}

}  // namespace

class HistorySearchToolTest : public testing::Test {
 protected:
  HistorySearchToolTest()
      : task_environment_(content::BrowserTaskEnvironment::IO_MAINLOOP) {}

  void EnableFeature() {
    feature_list_.InitAndEnableFeature(history_embeddings::kHistoryEmbeddings);
  }

  void DisableFeature() {
    feature_list_.InitAndDisableFeature(history_embeddings::kHistoryEmbeddings);
  }

  std::unique_ptr<HistorySearchTool> CreateTool() {
    return std::make_unique<HistorySearchTool>(&profile_);
  }

  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;
  TestingProfile profile_;
};

TEST_F(HistorySearchToolTest, RequiresUserInteractionBeforeHandling) {
  EnableFeature();
  auto tool = CreateTool();
  mojom::ToolUseEvent event(tool->Name().data(), "1", "{}", std::nullopt,
                            std::nullopt, nullptr, false);
  // Before permission is granted: returns a PermissionChallenge so the
  // user can confirm that history results may be sent to Brave AI. The
  // user-facing wording lives in `get_tool_permission_implications.tsx`,
  // so the challenge itself just needs to be a non-null sentinel here.
  auto result = tool->RequiresUserInteractionBeforeHandling(event);
  ASSERT_TRUE(std::holds_alternative<mojom::PermissionChallengePtr>(result));
  EXPECT_TRUE(std::get<mojom::PermissionChallengePtr>(result));

  // After permission is granted: no further interaction needed.
  tool->UserPermissionGranted("1");
  result = tool->RequiresUserInteractionBeforeHandling(event);
  ASSERT_TRUE(std::holds_alternative<bool>(result));
  EXPECT_FALSE(std::get<bool>(result));
}

TEST_F(HistorySearchToolTest, InvalidInputProducesError) {
  EnableFeature();
  auto tool = CreateTool();
  struct {
    std::string_view name;
    std::string_view input_json;
    std::string_view expected_error_substring;
  } kCases[] = {
      {"malformed json", "{ not json", "failed to parse"},
      {"missing query", R"({})", "missing or empty 'query'"},
      {"empty query", R"({"query": ""})", "missing or empty 'query'"},
      // Default search_query_minimum_word_count is 2.
      {"single-word query", R"({"query": "kittens"})", "at least 2 words"},
  };
  for (const auto& c : kCases) {
    SCOPED_TRACE(c.name);
    EXPECT_THAT(RunTool(tool.get(), std::string(c.input_json)),
                testing::HasSubstr(std::string(c.expected_error_substring)));
  }
}

TEST_F(HistorySearchToolTest, FeatureDisabledReturnsError) {
  DisableFeature();
  auto tool = CreateTool();
  EXPECT_THAT(RunTool(tool.get(), R"({"query": "anything at all"})"),
              testing::HasSubstr("not enabled"));
}

TEST_F(HistorySearchToolTest, InputPropertiesDeclareQueryRequired) {
  auto tool = CreateTool();
  auto required = tool->RequiredProperties();
  ASSERT_TRUE(required.has_value());
  EXPECT_THAT(*required, testing::Contains("query"));
}

TEST_F(HistorySearchToolTest, InputPropertiesDeclareOptionalProperties) {
  auto tool = CreateTool();
  auto props = tool->InputProperties();
  ASSERT_TRUE(props.has_value());
  EXPECT_TRUE(props->Find("query"));
  EXPECT_TRUE(props->Find("count"));
  EXPECT_TRUE(props->Find("time_range_start_days_ago"));
}

// The visit time set by MakeRow is base::Time::FromTimeT(1700000000),
// which TimeFormatAsIso8601 renders as this exact string.
constexpr char kExpectedLastVisitTime[] = "2023-11-14T22:13:20.000Z";

TEST(HistorySearchToolJsonTest, EmptyResultProducesEmptyResultsArray) {
  history_embeddings::SearchResult result;
  std::string json = internal::BuildHistorySearchResultJson(
      "kittens", result, /*include_all_passages=*/false);

  EXPECT_THAT(json,
              base::test::IsJson(R"({"query": "kittens", "results": []})"));
}

TEST(HistorySearchToolJsonTest, ResultsAreSerialized) {
  history_embeddings::SearchResult result;
  result.scored_url_rows.push_back(
      MakeRow("https://example.com/article", u"Article title"));

  std::string json = internal::BuildHistorySearchResultJson(
      "article", result, /*include_all_passages=*/false);

  // No passages were attached to the row, so neither 'passages' nor
  // 'snippet' is emitted -- otherwise BuildHistorySearchResultJson would
  // trigger GetBestPassage()'s CHECK.
  EXPECT_THAT(json, base::test::IsJson(base::StrCat({R"({
              "query": "article",
              "results": [{
                "title": "Article title",
                "url": "https://example.com/article",
                "last_visit_time": ")",
                                                     kExpectedLastVisitTime,
                                                     R"("
              }]
            })"})));
}

TEST(HistorySearchToolJsonTest, IncludeAllPassagesOrdersByScore) {
  history_embeddings::SearchResult result;
  auto row = MakeRow("https://example.com/article", u"Article title");
  AddPassage(row, "low score passage", /*score=*/0.1f, /*word_count=*/3);
  AddPassage(row, "high score passage", /*score=*/0.9f, /*word_count=*/3);
  AddPassage(row, "mid score passage", /*score=*/0.5f, /*word_count=*/3);
  result.scored_url_rows.push_back(std::move(row));

  std::string json = internal::BuildHistorySearchResultJson(
      "article", result, /*include_all_passages=*/true);

  EXPECT_THAT(json, base::test::IsJson(base::StrCat({R"({
              "query": "article",
              "results": [{
                "title": "Article title",
                "url": "https://example.com/article",
                "last_visit_time": ")",
                                                     kExpectedLastVisitTime,
                                                     R"(",
                "passages": [
                  "high score passage",
                  "mid score passage",
                  "low score passage"
                ]
              }]
            })"})));
}

TEST(HistorySearchToolJsonTest, BestPassageSnippetWhenNotIncludingAll) {
  history_embeddings::SearchResult result;
  auto row = MakeRow("https://example.com/article", u"Article title");
  AddPassage(row, "low score passage", /*score=*/0.1f, /*word_count=*/3);
  AddPassage(row, "high score passage", /*score=*/0.9f, /*word_count=*/3);
  result.scored_url_rows.push_back(std::move(row));

  std::string json = internal::BuildHistorySearchResultJson(
      "article", result, /*include_all_passages=*/false);

  EXPECT_THAT(json, base::test::IsJson(base::StrCat({R"({
              "query": "article",
              "results": [{
                "title": "Article title",
                "url": "https://example.com/article",
                "last_visit_time": ")",
                                                     kExpectedLastVisitTime,
                                                     R"(",
                "snippet": "high score passage"
              }]
            })"})));
}

}  // namespace ai_chat
