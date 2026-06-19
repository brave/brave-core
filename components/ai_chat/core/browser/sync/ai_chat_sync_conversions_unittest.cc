/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"

#include <optional>
#include <string>
#include <vector>

#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/sync/protocol/ai_chat_specifics.pb.h"
#include "components/sync/protocol/entity_data.h"
#include "components/sync/protocol/entity_specifics.pb.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {
namespace {

// A string long enough to cross kSyncCompressionThresholdBytes and highly
// repetitive, so gzip is guaranteed to shrink it.
std::string CompressibleString() {
  return std::string(2 * kSyncCompressionThresholdBytes, 'a');
}

}  // namespace

TEST(AIChatSyncConversionsTest, WriteCompressibleStringShortStaysRaw) {
  sync_pb::AIChatCompressibleString out;
  WriteCompressibleString("hello", &out);
  EXPECT_TRUE(out.has_raw());
  EXPECT_FALSE(out.has_gzipped());
  EXPECT_EQ(out.raw(), "hello");
  EXPECT_FALSE(out.was_truncated_for_sync());
}

TEST(AIChatSyncConversionsTest, WriteCompressibleStringBelowThresholdStaysRaw) {
  // Exactly one byte under the threshold must not be compressed.
  const std::string value(kSyncCompressionThresholdBytes - 1, 'a');
  sync_pb::AIChatCompressibleString out;
  WriteCompressibleString(value, &out);
  EXPECT_TRUE(out.has_raw());
  EXPECT_FALSE(out.has_gzipped());
  EXPECT_EQ(out.raw(), value);
}

TEST(AIChatSyncConversionsTest, WriteCompressibleStringLongGetsGzipped) {
  const std::string value = CompressibleString();
  sync_pb::AIChatCompressibleString out;
  WriteCompressibleString(value, &out);
  EXPECT_TRUE(out.has_gzipped());
  EXPECT_FALSE(out.has_raw());
  EXPECT_LT(out.gzipped().size(), value.size());
}

TEST(AIChatSyncConversionsTest, MarkCompressibleStringTruncatedSetsSentinel) {
  sync_pb::AIChatCompressibleString out;
  WriteCompressibleString("some value", &out);
  MarkCompressibleStringTruncated(&out);
  EXPECT_TRUE(out.was_truncated_for_sync());
  EXPECT_FALSE(out.has_raw());
  EXPECT_FALSE(out.has_gzipped());
}

TEST(AIChatSyncConversionsTest, ReadCompressibleStringRawRoundTrip) {
  sync_pb::AIChatCompressibleString in;
  in.set_raw("plain");
  EXPECT_EQ(ReadCompressibleString(in), "plain");
}

TEST(AIChatSyncConversionsTest, ReadCompressibleStringEmptyRawIsEmptyNotNull) {
  // An explicitly-set empty raw string is distinct from an unset field: it
  // reads back as "" rather than nullopt.
  sync_pb::AIChatCompressibleString in;
  in.set_raw("");
  std::optional<std::string> result = ReadCompressibleString(in);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(*result, "");
}

TEST(AIChatSyncConversionsTest, ReadCompressibleStringGzippedRoundTrip) {
  const std::string value = CompressibleString();
  sync_pb::AIChatCompressibleString wire;
  WriteCompressibleString(value, &wire);
  ASSERT_TRUE(wire.has_gzipped());
  EXPECT_EQ(ReadCompressibleString(wire), value);
}

TEST(AIChatSyncConversionsTest, ReadCompressibleStringTruncatedReturnsNullopt) {
  sync_pb::AIChatCompressibleString in;
  MarkCompressibleStringTruncated(&in);
  EXPECT_EQ(ReadCompressibleString(in), std::nullopt);
}

TEST(AIChatSyncConversionsTest, ReadCompressibleStringUnsetReturnsNullopt) {
  sync_pb::AIChatCompressibleString in;
  EXPECT_EQ(ReadCompressibleString(in), std::nullopt);
}

TEST(AIChatSyncConversionsTest, ReadCompressibleStringBadGzipReturnsNullopt) {
  sync_pb::AIChatCompressibleString in;
  in.set_gzipped("this is not valid gzip data");
  EXPECT_EQ(ReadCompressibleString(in), std::nullopt);
}

TEST(AIChatSyncConversionsTest, ConversationMetadataToSpecifics) {
  auto conversation = mojom::Conversation::New();
  conversation->uuid = "conv-1";
  conversation->title = "My conversation";
  conversation->model_key = "model-key";
  conversation->total_tokens = 1234;
  conversation->trimmed_tokens = 56;
  conversation->updated_time =
      base::Time::FromDeltaSinceWindowsEpoch(base::Microseconds(987654321));

  sync_pb::AIChatConversationSpecifics specifics =
      ConversationMetadataToSpecifics(*conversation);

  ASSERT_TRUE(specifics.has_conversation());
  EXPECT_FALSE(specifics.has_entry());
  const auto& meta = specifics.conversation();
  EXPECT_EQ(meta.uuid(), "conv-1");
  EXPECT_EQ(meta.title(), "My conversation");
  EXPECT_EQ(meta.model_key(), "model-key");
  EXPECT_EQ(meta.total_tokens(), 1234u);
  EXPECT_EQ(meta.trimmed_tokens(), 56u);
  EXPECT_EQ(meta.last_modified_time_unix_epoch_micros(), 987654321);
}

TEST(AIChatSyncConversionsTest, ConversationMetadataToSpecificsNoModelKey) {
  auto conversation = mojom::Conversation::New();
  conversation->uuid = "conv-1";
  conversation->title = "No model";
  conversation->model_key = std::nullopt;

  sync_pb::AIChatConversationSpecifics specifics =
      ConversationMetadataToSpecifics(*conversation);

  ASSERT_TRUE(specifics.has_conversation());
  EXPECT_FALSE(specifics.conversation().has_model_key());
}

TEST(AIChatSyncConversionsTest, EntryToSpecificsMapsFields) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-1";
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;
  entry->text = "the entry text";
  entry->prompt = "the prompt";
  entry->selected_text = "selected";
  entry->model_key = "model-key";
  entry->created_time =
      base::Time::FromDeltaSinceWindowsEpoch(base::Microseconds(42));

  sync_pb::AIChatConversationSpecifics specifics =
      EntryToSpecifics("conv-1", *entry, {});

  ASSERT_TRUE(specifics.has_entry());
  EXPECT_FALSE(specifics.has_conversation());
  const auto& proto = specifics.entry();
  EXPECT_EQ(proto.uuid(), "entry-1");
  EXPECT_EQ(proto.conversation_uuid(), "conv-1");
  EXPECT_EQ(proto.entry_text(), "the entry text");
  EXPECT_EQ(proto.prompt(), "the prompt");
  EXPECT_EQ(proto.selected_text(), "selected");
  EXPECT_EQ(proto.model_key(), "model-key");
  EXPECT_EQ(proto.character_type(),
            static_cast<int32_t>(mojom::CharacterType::HUMAN));
  EXPECT_EQ(proto.action_type(),
            static_cast<int32_t>(mojom::ActionType::QUERY));
  EXPECT_EQ(proto.date_unix_epoch_micros(), 42);
}

TEST(AIChatSyncConversionsTest, EntryToSpecificsFiltersAssociatedContent) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-1";
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;
  entry->created_time = base::Time::Now();

  // One content tied to this entry, one tied to a different entry, and one
  // with no entry association at all.
  std::vector<mojom::AssociatedContentPtr> content;

  auto mine = mojom::AssociatedContent::New();
  mine->uuid = "content-mine";
  mine->title = "Mine";
  mine->url = GURL("https://example.com/mine");
  mine->content_type = mojom::ContentType::PageContent;
  mine->content_used_percentage = 50;
  mine->conversation_turn_uuid = "entry-1";
  content.push_back(std::move(mine));

  auto other = mojom::AssociatedContent::New();
  other->uuid = "content-other";
  other->content_type = mojom::ContentType::PageContent;
  other->conversation_turn_uuid = "entry-2";
  content.push_back(std::move(other));

  auto unattached = mojom::AssociatedContent::New();
  unattached->uuid = "content-unattached";
  unattached->content_type = mojom::ContentType::PageContent;
  unattached->conversation_turn_uuid = std::nullopt;
  content.push_back(std::move(unattached));

  sync_pb::AIChatConversationSpecifics specifics =
      EntryToSpecifics("conv-1", *entry, content);

  ASSERT_TRUE(specifics.has_entry());
  ASSERT_EQ(specifics.entry().associated_content_size(), 1);
  const auto& proto_content = specifics.entry().associated_content(0);
  EXPECT_EQ(proto_content.uuid(), "content-mine");
  EXPECT_EQ(proto_content.title(), "Mine");
  EXPECT_EQ(proto_content.url(), "https://example.com/mine");
  EXPECT_EQ(proto_content.content_used_percentage(), 50);
}

TEST(AIChatSyncConversionsTest, EntryToSpecificsCompletionEventCompressed) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-1";
  entry->character_type = mojom::CharacterType::ASSISTANT;
  entry->action_type = mojom::ActionType::RESPONSE;
  entry->created_time = base::Time::Now();
  const std::string completion = CompressibleString();
  entry->events = std::vector<mojom::ConversationEntryEventPtr>{};
  entry->events->push_back(mojom::ConversationEntryEvent::NewCompletionEvent(
      mojom::CompletionEvent::New(completion)));

  sync_pb::AIChatConversationSpecifics specifics =
      EntryToSpecifics("conv-1", *entry, {});

  ASSERT_TRUE(specifics.has_entry());
  ASSERT_EQ(specifics.entry().events_size(), 1);
  const auto& event = specifics.entry().events(0);
  ASSERT_TRUE(event.has_completion());
  EXPECT_TRUE(event.completion().has_gzipped());
  EXPECT_EQ(ReadCompressibleString(event.completion()), completion);
}

TEST(AIChatSyncConversionsTest, EntryToSpecificsSearchQueriesEvent) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-1";
  entry->character_type = mojom::CharacterType::ASSISTANT;
  entry->action_type = mojom::ActionType::RESPONSE;
  entry->created_time = base::Time::Now();
  entry->events = std::vector<mojom::ConversationEntryEventPtr>{};
  entry->events->push_back(mojom::ConversationEntryEvent::NewSearchQueriesEvent(
      mojom::SearchQueriesEvent::New(
          std::vector<std::string>{"query one", "query two"})));

  sync_pb::AIChatConversationSpecifics specifics =
      EntryToSpecifics("conv-1", *entry, {});

  ASSERT_TRUE(specifics.has_entry());
  ASSERT_EQ(specifics.entry().events_size(), 1);
  const auto& event = specifics.entry().events(0);
  ASSERT_TRUE(event.has_search_queries());
  ASSERT_EQ(event.search_queries().queries_size(), 2);
  EXPECT_EQ(event.search_queries().queries(0), "query one");
  EXPECT_EQ(event.search_queries().queries(1), "query two");
}

TEST(AIChatSyncConversionsTest, CreateEntityDataFromSpecificsConversation) {
  sync_pb::AIChatConversationSpecifics specifics;
  specifics.mutable_conversation()->set_uuid("conv-1");

  auto entity_data = CreateEntityDataFromSpecifics(specifics);
  ASSERT_TRUE(entity_data);
  EXPECT_EQ(entity_data->name, "conversation:conv-1");
  ASSERT_TRUE(entity_data->specifics.has_ai_chat_conversation());
  EXPECT_EQ(entity_data->specifics.ai_chat_conversation().conversation().uuid(),
            "conv-1");
}

TEST(AIChatSyncConversionsTest, CreateEntityDataFromSpecificsEntry) {
  sync_pb::AIChatConversationSpecifics specifics;
  specifics.mutable_entry()->set_uuid("entry-1");

  auto entity_data = CreateEntityDataFromSpecifics(specifics);
  ASSERT_TRUE(entity_data);
  EXPECT_EQ(entity_data->name, "entry:entry-1");
  ASSERT_TRUE(entity_data->specifics.has_ai_chat_conversation());
  EXPECT_EQ(entity_data->specifics.ai_chat_conversation().entry().uuid(),
            "entry-1");
}

TEST(AIChatSyncConversionsTest, StorageKeyAndClientTagForConversation) {
  sync_pb::AIChatConversationSpecifics specifics;
  specifics.mutable_conversation()->set_uuid("conv-1");

  EXPECT_EQ(GetStorageKeyFromSpecifics(specifics), "c:conv-1");
  // The client tag is identical to the storage key.
  EXPECT_EQ(GetClientTagFromSpecifics(specifics),
            GetStorageKeyFromSpecifics(specifics));
}

TEST(AIChatSyncConversionsTest, StorageKeyAndClientTagForEntry) {
  sync_pb::AIChatConversationSpecifics specifics;
  specifics.mutable_entry()->set_uuid("entry-1");

  EXPECT_EQ(GetStorageKeyFromSpecifics(specifics), "e:entry-1");
  EXPECT_EQ(GetClientTagFromSpecifics(specifics),
            GetStorageKeyFromSpecifics(specifics));
}

TEST(AIChatSyncConversionsTest, StorageKeyEmptyForUnsetKind) {
  sync_pb::AIChatConversationSpecifics specifics;
  EXPECT_TRUE(GetStorageKeyFromSpecifics(specifics).empty());
}

TEST(AIChatSyncConversionsTest, GetStorageKeyFromEntitySpecifics) {
  sync_pb::AIChatConversationSpecifics specifics;
  specifics.mutable_entry()->set_uuid("entry-1");

  sync_pb::EntitySpecifics entity_specifics;
  *entity_specifics.mutable_ai_chat_conversation() = specifics;

  EXPECT_EQ(GetStorageKeyFromEntitySpecifics(entity_specifics), "e:entry-1");
}

}  // namespace ai_chat
