/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "base/hash/hash.h"
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
  EXPECT_FALSE(out.has_omitted_content_hash());
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

TEST(AIChatSyncConversionsTest, OmitCompressibleStringStoresContentHash) {
  sync_pb::AIChatCompressibleString out;
  WriteCompressibleString("some value", &out);
  OmitCompressibleString(&out);
  EXPECT_TRUE(out.has_omitted_content_hash());
  EXPECT_FALSE(out.has_raw());
  EXPECT_FALSE(out.has_gzipped());
  // The hash is over the original plaintext so a receiver can match it against
  // a local copy.
  EXPECT_EQ(out.omitted_content_hash(), base::PersistentHash("some value"));
}

TEST(AIChatSyncConversionsTest, OmitCompressibleStringHashesGzippedPlaintext) {
  // Omitting a value that was stored gzipped must hash the decompressed
  // plaintext, not the gzip bytes, so it matches the receiver's local copy.
  const std::string value = CompressibleString();
  sync_pb::AIChatCompressibleString out;
  WriteCompressibleString(value, &out);
  ASSERT_TRUE(out.has_gzipped());
  OmitCompressibleString(&out);
  EXPECT_EQ(out.omitted_content_hash(), base::PersistentHash(value));
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

TEST(AIChatSyncConversionsTest, ReadCompressibleStringOmittedReturnsNullopt) {
  sync_pb::AIChatCompressibleString in;
  WriteCompressibleString("some value", &in);
  OmitCompressibleString(&in);
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

TEST(AIChatSyncConversionsTest,
     ReadCompressibleStringRejectsOversizedDecompressedSize) {
  // A gzip stream's uncompressed-size trailer (ISIZE, the last 4 bytes) is
  // attacker-controlled; a tiny blob can claim a huge size. Reading must reject
  // a claim over kSyncCompressionMaxDecompressedBytes without decompressing.
  sync_pb::AIChatCompressibleString in;
  WriteCompressibleString(CompressibleString(), &in);
  ASSERT_TRUE(in.has_gzipped());

  std::string gzipped = in.gzipped();
  ASSERT_GE(gzipped.size(), 4u);
  const uint32_t forged_size =
      static_cast<uint32_t>(kSyncCompressionMaxDecompressedBytes + 1);
  gzipped[gzipped.size() - 4] = static_cast<char>(forged_size & 0xFF);
  gzipped[gzipped.size() - 3] = static_cast<char>((forged_size >> 8) & 0xFF);
  gzipped[gzipped.size() - 2] = static_cast<char>((forged_size >> 16) & 0xFF);
  gzipped[gzipped.size() - 1] = static_cast<char>((forged_size >> 24) & 0xFF);
  in.set_gzipped(gzipped);

  EXPECT_EQ(ReadCompressibleString(in), std::nullopt);
}

TEST(AIChatSyncConversionsTest, ConversationMetadataToSpecifics) {
  auto conversation = mojom::Conversation::New();
  conversation->uuid = "conv-1";
  conversation->title = "My conversation";
  conversation->model_key = "model-key";
  conversation->total_tokens = 1234;
  conversation->trimmed_tokens = 56;

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

TEST(AIChatSyncConversionsTest,
     EntryToSpecificsSyncsLatestEditUnderOriginalIdentity) {
  // An edited turn keeps its original text in |text| and stores each revision
  // in |edits| (most recent last). Sync must carry the latest edit's content,
  // but under the original turn's identity (uuid + created_time), so the sync
  // entity is stable across edits and keeps its conversation position.
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-1";
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;
  entry->text = "original text";
  entry->model_key = "original-model";
  entry->created_time =
      base::Time::FromDeltaSinceWindowsEpoch(base::Microseconds(42));

  auto first_edit = mojom::ConversationTurn::New();
  first_edit->uuid = "edit-1";
  first_edit->character_type = mojom::CharacterType::HUMAN;
  first_edit->action_type = mojom::ActionType::QUERY;
  first_edit->text = "first edit";
  first_edit->created_time =
      base::Time::FromDeltaSinceWindowsEpoch(base::Microseconds(100));

  auto latest_edit = mojom::ConversationTurn::New();
  latest_edit->uuid = "edit-2";
  latest_edit->character_type = mojom::CharacterType::HUMAN;
  latest_edit->action_type = mojom::ActionType::QUERY;
  latest_edit->text = "latest edit";
  latest_edit->model_key = "edited-model";
  latest_edit->created_time =
      base::Time::FromDeltaSinceWindowsEpoch(base::Microseconds(200));
  latest_edit->events = std::vector<mojom::ConversationEntryEventPtr>{};
  latest_edit->events->push_back(
      mojom::ConversationEntryEvent::NewCompletionEvent(
          mojom::CompletionEvent::New("latest answer")));

  entry->edits = std::vector<mojom::ConversationTurnPtr>{};
  entry->edits->push_back(std::move(first_edit));
  entry->edits->push_back(std::move(latest_edit));

  sync_pb::AIChatConversationSpecifics specifics =
      EntryToSpecifics("conv-1", *entry, {});

  ASSERT_TRUE(specifics.has_entry());
  const auto& proto = specifics.entry();
  // Identity: from the original turn.
  EXPECT_EQ(proto.uuid(), "entry-1");
  EXPECT_EQ(proto.date_unix_epoch_micros(), 42);
  // Content: from the most recent edit, not the original or intermediate edit.
  EXPECT_EQ(proto.entry_text(), "latest edit");
  EXPECT_EQ(proto.model_key(), "edited-model");
  ASSERT_EQ(proto.events_size(), 1);
  ASSERT_TRUE(proto.events(0).has_completion());
  EXPECT_EQ(ReadCompressibleString(proto.events(0).completion()),
            "latest answer");
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

TEST(AIChatSyncConversionsTest, GetStorageKeyFromEntitySpecifics) {
  sync_pb::AIChatConversationSpecifics specifics;
  specifics.mutable_entry()->set_uuid("entry-1");

  sync_pb::EntitySpecifics entity_specifics;
  *entity_specifics.mutable_ai_chat_conversation() = specifics;

  EXPECT_EQ(GetStorageKeyFromEntitySpecifics(entity_specifics), "e:entry-1");
}

TEST(AIChatSyncConversionsTest, EntryToSpecificsWebSourcesEvent) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-web";
  entry->created_time = base::Time::Now();
  entry->character_type = mojom::CharacterType::ASSISTANT;
  entry->action_type = mojom::ActionType::RESPONSE;

  auto source = mojom::WebSource::New();
  source->title = "Paris";
  source->url = GURL("https://en.wikipedia.org/wiki/Paris");
  source->favicon_url = GURL("https://en.wikipedia.org/favicon.ico");
  source->page_content = "Paris is the capital of France.";
  source->extra_snippets = std::vector<std::string>{"Snippet A", "Snippet B"};
  auto sources_event = mojom::WebSourcesEvent::New();
  sources_event->sources.push_back(std::move(source));
  std::vector<mojom::ConversationEntryEventPtr> events;
  events.push_back(
      mojom::ConversationEntryEvent::NewSourcesEvent(std::move(sources_event)));
  entry->events = std::move(events);

  auto specifics = EntryToSpecifics("conv-1", *entry, {});
  ASSERT_TRUE(specifics.has_entry());
  ASSERT_EQ(specifics.entry().events_size(), 1);
  const auto& event = specifics.entry().events(0);
  ASSERT_TRUE(event.has_web_sources());
  ASSERT_EQ(event.web_sources().sources_size(), 1);
  EXPECT_EQ(event.web_sources().sources(0).title(), "Paris");
  EXPECT_EQ(event.web_sources().sources(0).url(),
            "https://en.wikipedia.org/wiki/Paris");
  EXPECT_EQ(event.web_sources().sources(0).favicon_url(),
            "https://en.wikipedia.org/favicon.ico");
  EXPECT_EQ(
      ReadCompressibleString(event.web_sources().sources(0).page_content()),
      "Paris is the capital of France.");
  EXPECT_EQ(event.web_sources().sources(0).extra_snippets_size(), 2);
}

TEST(AIChatSyncConversionsTest, EntryToSpecificsToolUseEvent) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-tool";
  entry->created_time = base::Time::Now();
  entry->character_type = mojom::CharacterType::ASSISTANT;
  entry->action_type = mojom::ActionType::RESPONSE;

  auto tool_use = mojom::ToolUseEvent::New();
  tool_use->tool_name = "search";
  tool_use->id = "tool-1";
  tool_use->arguments_json = "{\"q\":\"test\"}";
  tool_use->is_server_result = true;
  std::vector<mojom::ContentBlockPtr> output;
  auto text = mojom::TextContentBlock::New();
  text->text = "result";
  output.push_back(mojom::ContentBlock::NewTextContentBlock(std::move(text)));
  tool_use->output = std::move(output);
  std::vector<mojom::ConversationEntryEventPtr> events;
  events.push_back(
      mojom::ConversationEntryEvent::NewToolUseEvent(std::move(tool_use)));
  entry->events = std::move(events);

  auto specifics = EntryToSpecifics("conv-1", *entry, {});
  ASSERT_TRUE(specifics.has_entry());
  ASSERT_EQ(specifics.entry().events_size(), 1);
  const auto& event = specifics.entry().events(0);
  ASSERT_TRUE(event.has_tool_use());
  EXPECT_EQ(event.tool_use().tool_name(), "search");
  EXPECT_EQ(event.tool_use().id(), "tool-1");
  EXPECT_TRUE(event.tool_use().is_server_result());
  EXPECT_EQ(ReadCompressibleString(event.tool_use().arguments_json()),
            "{\"q\":\"test\"}");
  ASSERT_EQ(event.tool_use().output_size(), 1);
  ASSERT_TRUE(event.tool_use().output(0).has_text_content_block());
  EXPECT_EQ(ReadCompressibleString(
                event.tool_use().output(0).text_content_block().text()),
            "result");
}

TEST(AIChatSyncConversionsTest, EntryToSpecificsUploadedFiles) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-files";
  entry->created_time = base::Time::Now();
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;

  std::vector<mojom::UploadedFilePtr> files;
  auto img = mojom::UploadedFile::New();
  img->filename = "cat.jpg";
  img->filesize = 5;
  img->type = mojom::UploadedFileType::kImage;
  img->data = {0x89, 0x50, 0x4e, 0x47, 0x0a};
  files.push_back(std::move(img));
  auto pdf = mojom::UploadedFile::New();
  pdf->filename = "spec.pdf";
  pdf->filesize = 0;
  pdf->type = mojom::UploadedFileType::kPdf;
  pdf->extracted_text = std::string(2048, 'x');  // Triggers gzip.
  files.push_back(std::move(pdf));
  entry->uploaded_files = std::move(files);

  auto specifics = EntryToSpecifics("conv-1", *entry, {});
  ASSERT_TRUE(specifics.has_entry());
  ASSERT_EQ(specifics.entry().uploaded_files_size(), 2);
  EXPECT_EQ(specifics.entry().uploaded_files(0).filename(), "cat.jpg");
  EXPECT_EQ(specifics.entry().uploaded_files(0).data().size(), 5u);
  EXPECT_TRUE(specifics.entry().uploaded_files(0).has_data());
  EXPECT_FALSE(specifics.entry().uploaded_files(0).has_omitted_data_hash());
  EXPECT_TRUE(
      specifics.entry().uploaded_files(1).extracted_text().has_gzipped());
}

TEST(AIChatSyncConversionsTest,
     EntryToSpecificsWritesSkillAndNearVerification) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-skill";
  entry->created_time = base::Time::Now();
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;
  entry->skill = mojom::SkillEntry::New("/summarize", "Summarize this page");
  entry->near_verification_status = mojom::NEARVerificationStatus::New(true);

  auto specifics = EntryToSpecifics("conv-1", *entry, {});
  ASSERT_TRUE(specifics.has_entry());
  const auto& proto = specifics.entry();
  ASSERT_TRUE(proto.has_skill());
  EXPECT_EQ(proto.skill().shortcut(), "/summarize");
  EXPECT_EQ(proto.skill().prompt(), "Summarize this page");
  ASSERT_TRUE(proto.has_near_verification_status());
  EXPECT_TRUE(proto.near_verification_status().verified());
}

TEST(AIChatSyncConversionsTest, EntryToSpecificsOmitsAbsentSkillAndNear) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-plain";
  entry->created_time = base::Time::Now();
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;

  auto specifics = EntryToSpecifics("conv-1", *entry, {});
  ASSERT_TRUE(specifics.has_entry());
  EXPECT_FALSE(specifics.entry().has_skill());
  EXPECT_FALSE(specifics.entry().has_near_verification_status());
}

}  // namespace ai_chat
