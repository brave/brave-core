// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/deep_research_parsing.h"

#include <string>

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

TEST(DeepResearchParsingTest, ParseQueriesEvent) {
  auto params = base::test::ParseJsonDict(R"({
    "queries": ["quantum computing", "AI breakthroughs"]
  })");

  auto result =
      ParseDeepResearchEvent("brave-chat.deepResearch.queries", params);

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->is_deep_research_event());
  ASSERT_TRUE(result->get_deep_research_event()->is_queries_event());

  const auto& event = result->get_deep_research_event()->get_queries_event();
  ASSERT_EQ(event->queries.size(), 2u);
  EXPECT_EQ(event->queries[0], "quantum computing");
  EXPECT_EQ(event->queries[1], "AI breakthroughs");
}

TEST(DeepResearchParsingTest, ParseQueriesEvent_MissingQueries) {
  auto params = base::test::ParseJsonDict(R"({})");

  auto result =
      ParseDeepResearchEvent("brave-chat.deepResearch.queries", params);

  EXPECT_FALSE(result);
}

TEST(DeepResearchParsingTest, ParseAnalyzingEvent) {
  auto params = base::test::ParseJsonDict(R"({
    "query": "test query",
    "urls": 10,
    "new_urls": 3
  })");

  auto result =
      ParseDeepResearchEvent("brave-chat.deepResearch.analyzing", params);

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->is_deep_research_event());
  ASSERT_TRUE(result->get_deep_research_event()->is_analyzing_event());

  const auto& event = result->get_deep_research_event()->get_analyzing_event();
  EXPECT_EQ(event->query, "test query");
  EXPECT_EQ(event->url_count, 10u);
  EXPECT_EQ(event->new_url_count, 3u);
}

TEST(DeepResearchParsingTest, ParseThinkingEvent) {
  auto params = base::test::ParseJsonDict(R"({
    "query": "deep thinking",
    "chunks_analyzed": 12,
    "chunks_selected": 4,
    "urls_analyzed": 8,
    "urls_selected": ["https://example.com/1", "https://example.com/2"],
    "urls_info": [
      {"url": "https://example.com/1", "favicon": "https://example.com/fav.ico"}
    ]
  })");

  auto result =
      ParseDeepResearchEvent("brave-chat.deepResearch.thinking", params);

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->is_deep_research_event());
  ASSERT_TRUE(result->get_deep_research_event()->is_thinking_event());

  const auto& event = result->get_deep_research_event()->get_thinking_event();
  EXPECT_EQ(event->query, "deep thinking");
  EXPECT_EQ(event->chunks_analyzed, 12u);
  EXPECT_EQ(event->chunks_selected, 4u);
  EXPECT_EQ(event->urls_analyzed, 8u);
  ASSERT_EQ(event->urls_selected.size(), 2u);
  EXPECT_EQ(event->urls_selected[0], GURL("https://example.com/1"));
  ASSERT_EQ(event->urls_info.size(), 1u);
  EXPECT_EQ(event->urls_info[0]->url, GURL("https://example.com/1"));
  EXPECT_EQ(event->urls_info[0]->favicon, GURL("https://example.com/fav.ico"));
}

TEST(DeepResearchParsingTest, ParseProgressEvent) {
  auto params = base::test::ParseJsonDict(R"({
    "elapsed_seconds": 45.5,
    "iterations": 2,
    "queries_count": 5,
    "urls_analyzed": 14,
    "snippets_analyzed": 28
  })");

  auto result =
      ParseDeepResearchEvent("brave-chat.deepResearch.progress", params);

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->is_deep_research_event());
  ASSERT_TRUE(result->get_deep_research_event()->is_progress_event());

  const auto& event = result->get_deep_research_event()->get_progress_event();
  EXPECT_DOUBLE_EQ(event->elapsed_seconds, 45.5);
  EXPECT_EQ(event->iteration_count, 2u);
  EXPECT_EQ(event->queries_count, 5u);
  EXPECT_EQ(event->urls_analyzed, 14u);
  EXPECT_EQ(event->snippets_analyzed, 28u);
}

TEST(DeepResearchParsingTest, ParseSearchStatusEvent_Started) {
  auto params = base::test::ParseJsonDict(R"({
    "status": "started",
    "query": "test search",
    "queryIndex": 1,
    "totalQueries": 5,
    "urlsFound": 0,
    "elapsedMs": 0
  })");

  auto result =
      ParseDeepResearchEvent("brave-chat.deepResearch.searchStatus", params);

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->is_deep_research_event());
  ASSERT_TRUE(result->get_deep_research_event()->is_search_status_event());

  const auto& event =
      result->get_deep_research_event()->get_search_status_event();
  EXPECT_EQ(event->status, mojom::DeepResearchSearchStatus::kStarted);
  EXPECT_EQ(event->query, "test search");
  EXPECT_EQ(event->query_index, 1u);
  EXPECT_EQ(event->total_queries, 5u);
}

TEST(DeepResearchParsingTest, ParseSearchStatusEvent_Completed) {
  auto params = base::test::ParseJsonDict(R"({
    "status": "completed",
    "query": "test search",
    "queryIndex": 1,
    "totalQueries": 5,
    "urlsFound": 12,
    "elapsedMs": 1500
  })");

  auto result =
      ParseDeepResearchEvent("brave-chat.deepResearch.searchStatus", params);

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->is_deep_research_event());
  ASSERT_TRUE(result->get_deep_research_event()->is_search_status_event());

  const auto& event =
      result->get_deep_research_event()->get_search_status_event();
  EXPECT_EQ(event->status, mojom::DeepResearchSearchStatus::kCompleted);
  EXPECT_EQ(event->urls_found, 12u);
  EXPECT_EQ(event->elapsed_ms, 1500u);
}

TEST(DeepResearchParsingTest, ParseSearchStatusEvent_UnknownStatus) {
  auto params = base::test::ParseJsonDict(R"({
    "status": "unknown_status",
    "query": "test search",
    "queryIndex": 1,
    "totalQueries": 5
  })");

  auto result =
      ParseDeepResearchEvent("brave-chat.deepResearch.searchStatus", params);

  EXPECT_FALSE(result);
}

TEST(DeepResearchParsingTest, ParseAnalysisStatusEvent) {
  auto params = base::test::ParseJsonDict(R"({
    "status": "progress",
    "query": "analysis test",
    "chunksAnalyzed": 5,
    "chunksTotal": 10
  })");

  auto result =
      ParseDeepResearchEvent("brave-chat.deepResearch.analysisStatus", params);

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->is_deep_research_event());
  ASSERT_TRUE(result->get_deep_research_event()->is_analysis_status_event());

  const auto& event =
      result->get_deep_research_event()->get_analysis_status_event();
  EXPECT_EQ(event->status, mojom::DeepResearchAnalysisStatus::kProgress);
  EXPECT_EQ(event->query, "analysis test");
  EXPECT_EQ(event->chunks_analyzed, 5u);
  EXPECT_EQ(event->chunks_total, 10u);
}

TEST(DeepResearchParsingTest, ParseAnalysisStatusEvent_UnknownStatus) {
  auto params = base::test::ParseJsonDict(R"({
    "status": "unknown_status",
    "query": "analysis test",
    "chunksAnalyzed": 5,
    "chunksTotal": 10
  })");

  auto result =
      ParseDeepResearchEvent("brave-chat.deepResearch.analysisStatus", params);

  EXPECT_FALSE(result);
}

TEST(DeepResearchParsingTest, ParseCompleteEvent) {
  auto params = base::test::ParseJsonDict(R"({
    "reason": "All iterations finished"
  })");

  auto result =
      ParseDeepResearchEvent("brave-chat.deepResearch.complete", params);

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->is_deep_research_event());
  ASSERT_TRUE(result->get_deep_research_event()->is_complete_event());
  EXPECT_EQ(result->get_deep_research_event()->get_complete_event()->reason,
            "All iterations finished");
}

TEST(DeepResearchParsingTest, ParseErrorEvent) {
  auto params = base::test::ParseJsonDict(R"({
    "error": "Service timeout"
  })");

  auto result = ParseDeepResearchEvent("brave-chat.deepResearch.error", params);

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->is_deep_research_event());
  ASSERT_TRUE(result->get_deep_research_event()->is_error_event());
  EXPECT_EQ(result->get_deep_research_event()->get_error_event()->error,
            "Service timeout");
}

TEST(DeepResearchParsingTest, ParseIterationCompleteEvent) {
  auto params = base::test::ParseJsonDict(R"({
    "iteration": 2,
    "totalIterations": 5,
    "queriesThisIteration": 3,
    "urlsAnalyzed": 15,
    "blindspotsIdentified": 2
  })");

  auto result = ParseDeepResearchEvent(
      "brave-chat.deepResearch.iterationComplete", params);

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->is_deep_research_event());
  ASSERT_TRUE(result->get_deep_research_event()->is_iteration_complete_event());

  const auto& event =
      result->get_deep_research_event()->get_iteration_complete_event();
  EXPECT_EQ(event->iteration, 2u);
  EXPECT_EQ(event->total_iterations, 5u);
  EXPECT_EQ(event->queries_this_iteration, 3u);
  EXPECT_EQ(event->urls_analyzed, 15u);
  EXPECT_EQ(event->blindspots_identified, 2u);
}

TEST(DeepResearchParsingTest, ParseBlindspotsEvent) {
  auto params = base::test::ParseJsonDict(R"({
    "blindspots": ["topic A", "topic B", "topic C"]
  })");

  auto result =
      ParseDeepResearchEvent("brave-chat.deepResearch.blindspots", params);

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->is_deep_research_event());
  ASSERT_TRUE(result->get_deep_research_event()->is_blindspots_event());

  const auto& event = result->get_deep_research_event()->get_blindspots_event();
  ASSERT_EQ(event->blindspots.size(), 3u);
  EXPECT_EQ(event->blindspots[0], "topic A");
  EXPECT_EQ(event->blindspots[1], "topic B");
  EXPECT_EQ(event->blindspots[2], "topic C");
}

TEST(DeepResearchParsingTest, ParseFetchStatusEvent) {
  auto params = base::test::ParseJsonDict(R"({
    "query": "fetch test",
    "urlsTotal": 20,
    "urlsFetched": 8
  })");

  auto result =
      ParseDeepResearchEvent("brave-chat.deepResearch.fetchStatus", params);

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->is_deep_research_event());
  ASSERT_TRUE(result->get_deep_research_event()->is_fetch_status_event());

  const auto& event =
      result->get_deep_research_event()->get_fetch_status_event();
  EXPECT_EQ(event->query, "fetch test");
  EXPECT_EQ(event->urls_total, 20u);
  EXPECT_EQ(event->urls_fetched, 8u);
}

TEST(DeepResearchParsingTest, UnknownEventType) {
  auto params = base::test::ParseJsonDict(R"({})");

  auto result =
      ParseDeepResearchEvent("brave-chat.deepResearch.unknown", params);

  EXPECT_FALSE(result);
}

TEST(DeepResearchParsingTest, DefaultValues) {
  // Test that missing fields get sensible defaults
  auto params = base::test::ParseJsonDict(R"({})");

  auto result =
      ParseDeepResearchEvent("brave-chat.deepResearch.analyzing", params);

  ASSERT_TRUE(result);
  ASSERT_TRUE(result->is_deep_research_event());
  ASSERT_TRUE(result->get_deep_research_event()->is_analyzing_event());

  const auto& event = result->get_deep_research_event()->get_analyzing_event();
  EXPECT_EQ(event->query, "");
  EXPECT_EQ(event->url_count, 0u);
  EXPECT_EQ(event->new_url_count, 0u);
}

}  // namespace ai_chat
