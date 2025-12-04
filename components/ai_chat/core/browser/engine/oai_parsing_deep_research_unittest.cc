// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/engine/oai_parsing.h"

#include <string>

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

// =============================================================================
// Basic Type Handling Tests
// =============================================================================

TEST(OaiParsingDeepResearchTest, ReturnsNulloptForMissingType) {
  constexpr char kEventJson[] = R"({
    "research": {
      "event": "analyzing"
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  EXPECT_FALSE(result.has_value());
}

TEST(OaiParsingDeepResearchTest, ReturnsNulloptForUnknownType) {
  constexpr char kEventJson[] = R"({
    "type": "unknown_type"
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  EXPECT_FALSE(result.has_value());
}

TEST(OaiParsingDeepResearchTest, ResearchStartType) {
  constexpr char kEventJson[] = R"({
    "type": "research_start"
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->first->is_search_status_event());
}

// =============================================================================
// "research" Container - Nested Events
// =============================================================================

TEST(OaiParsingDeepResearchTest, AnalyzingEvent) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "analyzing"
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  ASSERT_TRUE(result.has_value());
  EXPECT_TRUE(result->first->is_search_status_event());
}

TEST(OaiParsingDeepResearchTest, ThinkingEvent_WithAllFields) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "thinking",
      "query": "test query",
      "urls_analyzed": 5,
      "urls_selected": ["https://example.com", "https://test.org"],
      "urls_info": [
        {"url": "https://example.com", "favicon": "https://example.com/favicon.ico"},
        {"url": "https://test.org", "favicon": "https://test.org/icon.png"}
      ]
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->first->is_thinking_event());

  const auto& thinking = result->first->get_thinking_event();
  EXPECT_EQ(thinking->query, "test query");
  EXPECT_EQ(thinking->urls_analyzed, 5);
  EXPECT_EQ(thinking->urls_selected.size(), 2u);
  EXPECT_EQ(thinking->urls_selected[0], "https://example.com");
  EXPECT_EQ(thinking->urls_info.size(), 2u);
  EXPECT_EQ(thinking->urls_info[0]->url, "https://example.com");
  EXPECT_EQ(thinking->urls_info[0]->favicon, "https://example.com/favicon.ico");
}

TEST(OaiParsingDeepResearchTest, ThinkingEvent_MinimalFields) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "thinking"
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->first->is_thinking_event());

  const auto& thinking = result->first->get_thinking_event();
  EXPECT_EQ(thinking->query, "");
  EXPECT_EQ(thinking->urls_analyzed, 0);
  EXPECT_TRUE(thinking->urls_selected.empty());
  EXPECT_TRUE(thinking->urls_info.empty());
}

TEST(OaiParsingDeepResearchTest, QueriesEvent_Valid) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "queries",
      "queries": ["search query 1", "search query 2"]
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->first->is_search_queries_event());

  const auto& queries = result->first->get_search_queries_event();
  EXPECT_EQ(queries->search_queries.size(), 2u);
  EXPECT_EQ(queries->search_queries[0], "search query 1");
  EXPECT_EQ(queries->search_queries[1], "search query 2");
}

TEST(OaiParsingDeepResearchTest, QueriesEvent_Empty) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "queries",
      "queries": []
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  // Empty queries should return nullopt
  EXPECT_FALSE(result.has_value());
}

TEST(OaiParsingDeepResearchTest, AnswerEvent_Valid) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "answer",
      "answer": "This is the research answer."
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->first->is_completion_event());

  const auto& completion = result->first->get_completion_event();
  EXPECT_EQ(completion->completion, "This is the research answer.");
}

TEST(OaiParsingDeepResearchTest, AnswerEvent_Empty) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "answer",
      "answer": ""
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  // Empty answer should return nullopt
  EXPECT_FALSE(result.has_value());
}

TEST(OaiParsingDeepResearchTest, InsightsEvent_ValidUrls) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "insights",
      "insights": {
        "https://example.com/article": "some content",
        "https://test.org/page": "more content"
      }
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->first->is_sources_event());

  const auto& sources = result->first->get_sources_event();
  EXPECT_EQ(sources->sources.size(), 2u);
}

TEST(OaiParsingDeepResearchTest, InsightsEvent_InvalidUrls) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "insights",
      "insights": {
        "not-a-valid-url": "content",
        "also-invalid": "more content"
      }
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  // All URLs invalid should return nullopt
  EXPECT_FALSE(result.has_value());
}

TEST(OaiParsingDeepResearchTest, VideosEvent_Valid) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "videos",
      "videos": [
        {
          "title": "Test Video",
          "url": "https://youtube.com/watch?v=123",
          "thumbnail_url": "https://img.youtube.com/vi/123/0.jpg",
          "age": "2 days ago",
          "description": "A test video",
          "duration": "10:30",
          "creator": "Test Creator",
          "publisher": "YouTube"
        }
      ]
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->first->is_video_results_event());

  const auto& videos = result->first->get_video_results_event();
  EXPECT_EQ(videos->videos.size(), 1u);
  EXPECT_EQ(videos->videos[0]->title, "Test Video");
  EXPECT_EQ(videos->videos[0]->url.spec(), "https://youtube.com/watch?v=123");
}

TEST(OaiParsingDeepResearchTest, ImagesEvent_Valid) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "images",
      "images": [
        {
          "title": "Test Image",
          "url": "https://example.com/image.jpg",
          "thumbnail_url": "https://example.com/thumb.jpg",
          "width": 1920,
          "height": 1080
        }
      ]
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->first->is_image_results_event());

  const auto& images = result->first->get_image_results_event();
  EXPECT_EQ(images->images.size(), 1u);
  EXPECT_EQ(images->images[0]->title, "Test Image");
  EXPECT_EQ(images->images[0]->image_url.spec(),
            "https://example.com/image.jpg");
}

TEST(OaiParsingDeepResearchTest, NewsEvent_Valid) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "news",
      "news": [
        {
          "title": "Breaking News Story",
          "url": "https://news.example.com/story",
          "thumbnail_url": "https://news.example.com/thumb.jpg",
          "favicon": "https://news.example.com/favicon.ico",
          "age": "1 hour ago",
          "source": "Example News",
          "is_breaking": true
        }
      ]
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->first->is_news_results_event());

  const auto& news = result->first->get_news_results_event();
  EXPECT_EQ(news->news.size(), 1u);
  EXPECT_EQ(news->news[0]->title, "Breaking News Story");
  EXPECT_EQ(news->news[0]->url.spec(), "https://news.example.com/story");
}

TEST(OaiParsingDeepResearchTest, DiscussionsEvent_Valid) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "discussions",
      "discussions": [
        {
          "title": "Discussion Thread",
          "url": "https://forum.example.com/thread/123",
          "description": "A discussion about testing",
          "favicon": "https://forum.example.com/favicon.ico",
          "age": "3 days ago",
          "forum_name": "Test Forum",
          "num_answers": 42
        }
      ]
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->first->is_discussion_results_event());

  const auto& discussions = result->first->get_discussion_results_event();
  EXPECT_EQ(discussions->discussions.size(), 1u);
  EXPECT_EQ(discussions->discussions[0]->title, "Discussion Thread");
  EXPECT_EQ(discussions->discussions[0]->url.spec(),
            "https://forum.example.com/thread/123");
}

TEST(OaiParsingDeepResearchTest, BlindspotsEvent_Valid) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "blindspots",
      "blindspots": ["Missing perspective 1", "Unexplored angle 2"]
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->first->is_blindspots_event());

  const auto& blindspots = result->first->get_blindspots_event();
  EXPECT_EQ(blindspots->blindspots.size(), 2u);
  EXPECT_EQ(blindspots->blindspots[0], "Missing perspective 1");
  EXPECT_EQ(blindspots->blindspots[1], "Unexplored angle 2");
}

TEST(OaiParsingDeepResearchTest, BlindspotsEvent_Empty) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "blindspots",
      "blindspots": []
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  // Empty blindspots should return nullopt
  EXPECT_FALSE(result.has_value());
}

TEST(OaiParsingDeepResearchTest, ProgressEvent) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "progress",
      "iteration": 3,
      "elapsed_seconds": 45.5,
      "urls_analyzed": 25,
      "queries_issued": 8
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->first->is_progress_event());

  const auto& progress = result->first->get_progress_event();
  EXPECT_EQ(progress->iteration, 3);
  EXPECT_DOUBLE_EQ(progress->elapsed_seconds, 45.5);
  EXPECT_EQ(progress->urls_analyzed, 25);
  EXPECT_EQ(progress->queries_issued, 8);
}

TEST(OaiParsingDeepResearchTest, PingEvent_ReturnsNullopt) {
  constexpr char kEventJson[] = R"({
    "type": "research",
    "research": {
      "event": "ping"
    }
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  // Ping events should be ignored
  EXPECT_FALSE(result.has_value());
}

// =============================================================================
// Top-level Events
// =============================================================================

TEST(OaiParsingDeepResearchTest, CompletionEvent_Valid) {
  constexpr char kEventJson[] = R"({
    "type": "completion",
    "completion": "This is a completion text."
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->first->is_completion_event());

  const auto& completion = result->first->get_completion_event();
  EXPECT_EQ(completion->completion, "This is a completion text.");
}

TEST(OaiParsingDeepResearchTest, ConversationTitleEvent_Valid) {
  constexpr char kEventJson[] = R"({
    "type": "conversationTitle",
    "title": "My Conversation Title"
  })";

  base::Value::Dict event_dict = base::test::ParseJsonDict(kEventJson);
  auto result = ParseResearchEvent(event_dict, nullptr);

  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result->first->is_conversation_title_event());

  const auto& title_event = result->first->get_conversation_title_event();
  EXPECT_EQ(title_event->title, "My Conversation Title");
}

}  // namespace ai_chat
