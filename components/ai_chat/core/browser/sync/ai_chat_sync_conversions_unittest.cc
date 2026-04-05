// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"

#include <string>
#include <vector>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "brave/components/sync/protocol/ai_chat_specifics.pb.h"
#include "components/sync/protocol/entity_data.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {
namespace {

mojom::ConversationPtr MakeConversation() {
  auto conv = mojom::Conversation::New();
  conv->uuid = "conv-uuid-123";
  conv->title = "Test Conversation";
  conv->model_key = "claude-3";
  conv->total_tokens = 500;
  conv->trimmed_tokens = 100;

  auto content = mojom::AssociatedContent::New();
  content->uuid = "content-uuid-1";
  content->title = "Test Page";
  content->url = GURL("https://example.com/page");
  content->content_type = mojom::ContentType::PageContent;
  content->content_used_percentage = 75;
  content->conversation_turn_uuid = "entry-uuid-1";
  conv->associated_content.push_back(std::move(content));

  return conv;
}

mojom::ConversationTurnPtr MakeHumanEntry() {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-uuid-1";
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;
  entry->text = "What is Brave browser?";
  entry->selected_text = "Brave";
  entry->created_time = base::Time::Now();
  return entry;
}

mojom::ConversationTurnPtr MakeAssistantEntryWithAllEvents() {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-uuid-2";
  entry->character_type = mojom::CharacterType::ASSISTANT;
  entry->action_type = mojom::ActionType::RESPONSE;
  entry->text = "Brave is a privacy-focused browser.";
  entry->model_key = "claude-3";
  entry->created_time = base::Time::Now();

  std::vector<mojom::ConversationEntryEventPtr> events;

  // CompletionEvent
  auto completion = mojom::CompletionEvent::New();
  completion->completion = "Brave is a privacy-focused browser.";
  events.push_back(
      mojom::ConversationEntryEvent::NewCompletionEvent(std::move(completion)));

  // SearchQueriesEvent
  auto search = mojom::SearchQueriesEvent::New();
  search->search_queries = {"brave browser", "privacy browser"};
  events.push_back(
      mojom::ConversationEntryEvent::NewSearchQueriesEvent(std::move(search)));

  // WebSourcesEvent
  auto sources = mojom::WebSourcesEvent::New();
  auto source = mojom::WebSource::New();
  source->title = "Brave Homepage";
  source->url = GURL("https://brave.com");
  source->favicon_url = GURL("https://brave.com/favicon.ico");
  sources->sources.push_back(std::move(source));
  sources->rich_results = {"{\"type\":\"test\"}"};
  events.push_back(
      mojom::ConversationEntryEvent::NewSourcesEvent(std::move(sources)));

  // InlineSearchEvent
  auto inline_search = mojom::InlineSearchEvent::New();
  inline_search->query = "brave features";
  inline_search->results_json = R"({"results": ["shields", "rewards"]})";
  events.push_back(mojom::ConversationEntryEvent::NewInlineSearchEvent(
      std::move(inline_search)));

  // ToolUseEvent
  auto tool_use = mojom::ToolUseEvent::New();
  tool_use->tool_name = "web_search";
  tool_use->id = "tool-call-1";
  tool_use->arguments_json = R"({"query": "brave browser"})";
  events.push_back(
      mojom::ConversationEntryEvent::NewToolUseEvent(std::move(tool_use)));

  entry->events = std::move(events);
  return entry;
}

TEST(AIChatSyncConversionsTest, RoundTripConversationMetadata) {
  auto original = MakeConversation();
  auto archive = mojom::ConversationArchive::New();
  archive->entries.push_back(MakeHumanEntry());

  auto specifics = ConversationToSpecifics(*original, *archive);

  auto roundtripped = SpecificsToConversation(specifics);
  EXPECT_EQ(roundtripped->uuid, original->uuid);
  EXPECT_EQ(roundtripped->title, original->title);
  EXPECT_EQ(roundtripped->model_key, original->model_key);
  EXPECT_EQ(roundtripped->total_tokens, original->total_tokens);
  EXPECT_EQ(roundtripped->trimmed_tokens, original->trimmed_tokens);
}

TEST(AIChatSyncConversionsTest, RoundTripAssociatedContent) {
  auto original = MakeConversation();
  auto archive = mojom::ConversationArchive::New();

  auto specifics = ConversationToSpecifics(*original, *archive);
  auto roundtripped = SpecificsToConversation(specifics);

  ASSERT_EQ(roundtripped->associated_content.size(), 1u);
  const auto& content = roundtripped->associated_content[0];
  EXPECT_EQ(content->uuid, "content-uuid-1");
  EXPECT_EQ(content->title, "Test Page");
  EXPECT_EQ(content->url, GURL("https://example.com/page"));
  EXPECT_EQ(content->content_type, mojom::ContentType::PageContent);
  EXPECT_EQ(content->content_used_percentage, 75);
  EXPECT_EQ(content->conversation_turn_uuid, "entry-uuid-1");
}

TEST(AIChatSyncConversionsTest, RoundTripHumanEntry) {
  auto conv = MakeConversation();
  auto archive = mojom::ConversationArchive::New();
  archive->entries.push_back(MakeHumanEntry());

  auto specifics = ConversationToSpecifics(*conv, *archive);
  auto roundtripped = SpecificsToArchive(specifics);

  ASSERT_EQ(roundtripped->entries.size(), 1u);
  const auto& entry = roundtripped->entries[0];
  ASSERT_TRUE(entry->uuid.has_value());
  EXPECT_EQ(*entry->uuid, "entry-uuid-1");
  EXPECT_EQ(entry->character_type, mojom::CharacterType::HUMAN);
  EXPECT_EQ(entry->action_type, mojom::ActionType::QUERY);
  EXPECT_EQ(entry->text, "What is Brave browser?");
  EXPECT_EQ(entry->selected_text, "Brave");
}

TEST(AIChatSyncConversionsTest, RoundTripCompletionEvent) {
  auto conv = MakeConversation();
  auto archive = mojom::ConversationArchive::New();
  archive->entries.push_back(MakeAssistantEntryWithAllEvents());

  auto specifics = ConversationToSpecifics(*conv, *archive);
  auto roundtripped = SpecificsToArchive(specifics);

  ASSERT_EQ(roundtripped->entries.size(), 1u);
  ASSERT_TRUE(roundtripped->entries[0]->events.has_value());
  const auto& events = *roundtripped->entries[0]->events;
  ASSERT_GE(events.size(), 1u);
  ASSERT_TRUE(events[0]->is_completion_event());
  EXPECT_EQ(events[0]->get_completion_event()->completion,
            "Brave is a privacy-focused browser.");
}

TEST(AIChatSyncConversionsTest, RoundTripSearchQueriesEvent) {
  auto conv = MakeConversation();
  auto archive = mojom::ConversationArchive::New();
  archive->entries.push_back(MakeAssistantEntryWithAllEvents());

  auto specifics = ConversationToSpecifics(*conv, *archive);
  auto roundtripped = SpecificsToArchive(specifics);

  ASSERT_TRUE(roundtripped->entries[0]->events.has_value());
  const auto& events = *roundtripped->entries[0]->events;
  ASSERT_GE(events.size(), 2u);
  ASSERT_TRUE(events[1]->is_search_queries_event());
  const auto& queries = events[1]->get_search_queries_event()->search_queries;
  ASSERT_EQ(queries.size(), 2u);
  EXPECT_EQ(queries[0], "brave browser");
  EXPECT_EQ(queries[1], "privacy browser");
}

TEST(AIChatSyncConversionsTest, RoundTripWebSourcesEvent) {
  auto conv = MakeConversation();
  auto archive = mojom::ConversationArchive::New();
  archive->entries.push_back(MakeAssistantEntryWithAllEvents());

  auto specifics = ConversationToSpecifics(*conv, *archive);
  auto roundtripped = SpecificsToArchive(specifics);

  ASSERT_TRUE(roundtripped->entries[0]->events.has_value());
  const auto& events = *roundtripped->entries[0]->events;
  ASSERT_GE(events.size(), 3u);
  ASSERT_TRUE(events[2]->is_sources_event());
  const auto& sources = events[2]->get_sources_event();
  ASSERT_EQ(sources->sources.size(), 1u);
  EXPECT_EQ(sources->sources[0]->title, "Brave Homepage");
  EXPECT_EQ(sources->sources[0]->url, GURL("https://brave.com"));
  ASSERT_EQ(sources->rich_results.size(), 1u);
  EXPECT_EQ(sources->rich_results[0], "{\"type\":\"test\"}");
}

TEST(AIChatSyncConversionsTest, RoundTripInlineSearchEvent) {
  auto conv = MakeConversation();
  auto archive = mojom::ConversationArchive::New();
  archive->entries.push_back(MakeAssistantEntryWithAllEvents());

  auto specifics = ConversationToSpecifics(*conv, *archive);
  auto roundtripped = SpecificsToArchive(specifics);

  ASSERT_TRUE(roundtripped->entries[0]->events.has_value());
  const auto& events = *roundtripped->entries[0]->events;
  ASSERT_GE(events.size(), 4u);
  ASSERT_TRUE(events[3]->is_inline_search_event());
  const auto& inline_search = events[3]->get_inline_search_event();
  EXPECT_EQ(inline_search->query, "brave features");
  EXPECT_EQ(inline_search->results_json,
            R"({"results": ["shields", "rewards"]})");
}

TEST(AIChatSyncConversionsTest, RoundTripToolUseEvent) {
  auto conv = MakeConversation();
  auto archive = mojom::ConversationArchive::New();
  archive->entries.push_back(MakeAssistantEntryWithAllEvents());

  auto specifics = ConversationToSpecifics(*conv, *archive);
  auto roundtripped = SpecificsToArchive(specifics);

  ASSERT_TRUE(roundtripped->entries[0]->events.has_value());
  const auto& events = *roundtripped->entries[0]->events;
  ASSERT_GE(events.size(), 5u);
  ASSERT_TRUE(events[4]->is_tool_use_event());
  const auto& tool_use = events[4]->get_tool_use_event();
  EXPECT_EQ(tool_use->tool_name, "web_search");
  EXPECT_EQ(tool_use->id, "tool-call-1");
  EXPECT_EQ(tool_use->arguments_json, R"({"query": "brave browser"})");
}

TEST(AIChatSyncConversionsTest, StorageKeyFromSpecifics) {
  sync_pb::AIChatConversationSpecifics specifics;
  specifics.set_uuid("test-uuid");
  EXPECT_EQ(GetStorageKeyFromSpecifics(specifics), "test-uuid");
  EXPECT_EQ(GetClientTagFromSpecifics(specifics), "test-uuid");
}

TEST(AIChatSyncConversionsTest, CreateEntityData) {
  sync_pb::AIChatConversationSpecifics specifics;
  specifics.set_uuid("test-uuid");
  specifics.set_title("Test");

  auto entity_data = CreateEntityDataFromSpecifics(specifics);
  EXPECT_EQ(entity_data->name, "test-uuid");
  EXPECT_TRUE(entity_data->specifics.has_ai_chat_conversation());
  EXPECT_EQ(entity_data->specifics.ai_chat_conversation().uuid(), "test-uuid");
}

TEST(AIChatSyncConversionsTest, EmptyConversation) {
  auto conv = mojom::Conversation::New();
  conv->uuid = "empty-conv";
  auto archive = mojom::ConversationArchive::New();

  auto specifics = ConversationToSpecifics(*conv, *archive);
  EXPECT_EQ(specifics.entries_size(), 0);
  EXPECT_EQ(specifics.associated_content_size(), 0);

  auto roundtripped = SpecificsToConversation(specifics);
  EXPECT_EQ(roundtripped->uuid, "empty-conv");
}

TEST(AIChatSyncConversionsTest, MissingOptionalFields) {
  sync_pb::AIChatConversationSpecifics specifics;
  specifics.set_uuid("minimal");

  auto conv = SpecificsToConversation(specifics);
  EXPECT_EQ(conv->uuid, "minimal");
  EXPECT_FALSE(conv->model_key.has_value());

  auto archive = SpecificsToArchive(specifics);
  EXPECT_TRUE(archive->entries.empty());
}

TEST(AIChatSyncConversionsTest, EmptyWebSourcesEventRoundTrips) {
  sync_pb::AIChatConversationSpecifics specifics;
  specifics.set_uuid("empty-sources-test");
  auto* entry_proto = specifics.add_entries();
  entry_proto->set_uuid("entry-1");
  entry_proto->set_entry_text("Test");
  entry_proto->set_character_type(0);

  // Add a completion event.
  auto* completion_event = entry_proto->add_events();
  completion_event->set_event_order(0);
  completion_event->set_completion_text("Valid completion");

  // Add an empty web_sources event (no sources).
  auto* empty_sources = entry_proto->add_events();
  empty_sources->set_event_order(1);
  empty_sources->mutable_web_sources();

  auto archive = SpecificsToArchive(specifics);
  ASSERT_EQ(archive->entries.size(), 1u);
  ASSERT_TRUE(archive->entries[0]->events.has_value());
  // Both events round-trip: the completion and the empty web sources.
  ASSERT_EQ(archive->entries[0]->events->size(), 2u);
  EXPECT_TRUE(archive->entries[0]->events->at(0)->is_completion_event());
  EXPECT_TRUE(archive->entries[0]->events->at(1)->is_sources_event());
  EXPECT_TRUE(
      archive->entries[0]->events->at(1)->get_sources_event()->sources.empty());
}

}  // namespace
}  // namespace ai_chat
