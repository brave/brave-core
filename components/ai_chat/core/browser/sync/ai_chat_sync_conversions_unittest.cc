/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/hash/hash.h"
#include "base/numerics/byte_conversions.h"
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

TEST(AIChatSyncConversionsTest, OmitUploadedFileDataStoresDataHash) {
  sync_pb::AIChatUploadedFile file;
  const std::string bytes = "some raw uploaded file bytes";
  file.set_data(bytes);
  OmitUploadedFileData(&file);
  EXPECT_FALSE(file.has_data());
  EXPECT_TRUE(file.has_omitted_data_hash());
  // The hash is over the original bytes so a receiver can match it against a
  // local copy.
  EXPECT_EQ(file.omitted_data_hash(), base::PersistentHash(bytes));
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
  base::as_writable_byte_span(gzipped).last<4u>().copy_from(
      base::U32ToLittleEndian(
          static_cast<uint32_t>(kSyncCompressionMaxDecompressedBytes + 1)));
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

TEST(AIChatSyncConversionsTest, ConversationMetadataRoundTrip) {
  auto original = mojom::Conversation::New();
  original->uuid = "conv-123";
  original->title = "Test conversation";
  original->model_key = "claude-opus";
  original->total_tokens = 4096;
  original->trimmed_tokens = 256;

  auto specifics = ConversationMetadataToSpecifics(*original);
  ASSERT_TRUE(specifics.has_conversation());
  EXPECT_FALSE(specifics.has_entry());
  EXPECT_EQ(specifics.conversation().uuid(), "conv-123");
  EXPECT_EQ(specifics.conversation().title(), "Test conversation");
  EXPECT_EQ(specifics.conversation().model_key(), "claude-opus");
  EXPECT_EQ(specifics.conversation().total_tokens(), 4096u);
  EXPECT_EQ(specifics.conversation().trimmed_tokens(), 256u);

  auto rebuilt = SpecificsToConversationMetadata(specifics);
  ASSERT_TRUE(rebuilt);
  EXPECT_EQ(rebuilt->uuid, "conv-123");
  EXPECT_EQ(rebuilt->title, "Test conversation");
  ASSERT_TRUE(rebuilt->model_key.has_value());
  EXPECT_EQ(*rebuilt->model_key, "claude-opus");
  EXPECT_EQ(rebuilt->total_tokens, 4096u);
  EXPECT_EQ(rebuilt->trimmed_tokens, 256u);
}

TEST(AIChatSyncConversionsTest, ConversationMetadataOptionalFields) {
  auto original = mojom::Conversation::New();
  original->uuid = "conv-min";
  original->title = "Minimal";
  original->total_tokens = 0;
  original->trimmed_tokens = 0;

  auto specifics = ConversationMetadataToSpecifics(*original);
  auto rebuilt = SpecificsToConversationMetadata(specifics);
  ASSERT_TRUE(rebuilt);
  EXPECT_FALSE(rebuilt->model_key.has_value());
}

TEST(AIChatSyncConversionsTest, EntryRoundTripBasic) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-1";
  entry->created_time = base::Time::Now();
  entry->text = "What is the capital of France?";
  entry->prompt = "Be concise.";
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;
  entry->selected_text = "France";
  entry->model_key = "claude-opus";

  auto specifics = EntryToSpecifics("conv-1", *entry, {});
  ASSERT_TRUE(specifics.has_entry());
  EXPECT_EQ(specifics.entry().uuid(), "entry-1");
  EXPECT_EQ(specifics.entry().conversation_uuid(), "conv-1");
  EXPECT_EQ(specifics.entry().entry_text(), "What is the capital of France?");
  EXPECT_EQ(specifics.entry().prompt(), "Be concise.");
  EXPECT_EQ(specifics.entry().character_type(),
            static_cast<int>(mojom::CharacterType::HUMAN));
  EXPECT_EQ(specifics.entry().selected_text(), "France");
  EXPECT_EQ(specifics.entry().model_key(), "claude-opus");

  std::vector<mojom::AssociatedContentPtr> rebuilt_content;
  auto rebuilt = SpecificsToEntry(specifics, &rebuilt_content);
  ASSERT_TRUE(rebuilt);
  ASSERT_TRUE(rebuilt->uuid.has_value());
  EXPECT_EQ(*rebuilt->uuid, "entry-1");
  EXPECT_EQ(rebuilt->text, "What is the capital of France?");
  ASSERT_TRUE(rebuilt->prompt.has_value());
  EXPECT_EQ(*rebuilt->prompt, "Be concise.");
  EXPECT_EQ(rebuilt->character_type, mojom::CharacterType::HUMAN);
  EXPECT_EQ(rebuilt->action_type, mojom::ActionType::QUERY);
  ASSERT_TRUE(rebuilt->selected_text.has_value());
  EXPECT_EQ(*rebuilt->selected_text, "France");
  ASSERT_TRUE(rebuilt->model_key.has_value());
  EXPECT_EQ(*rebuilt->model_key, "claude-opus");
  EXPECT_TRUE(rebuilt_content.empty());
}

TEST(AIChatSyncConversionsTest, EntryRoundTripCompletionEvent) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-completion";
  entry->created_time = base::Time::Now();
  entry->character_type = mojom::CharacterType::ASSISTANT;
  entry->action_type = mojom::ActionType::RESPONSE;

  std::vector<mojom::ConversationEntryEventPtr> events;
  auto completion = mojom::CompletionEvent::New();
  completion->completion = "Paris.";
  events.push_back(
      mojom::ConversationEntryEvent::NewCompletionEvent(std::move(completion)));
  entry->events = std::move(events);

  auto specifics = EntryToSpecifics("conv-1", *entry, {});
  ASSERT_TRUE(specifics.has_entry());
  ASSERT_EQ(specifics.entry().events_size(), 1);
  ASSERT_TRUE(specifics.entry().events(0).has_completion());
  EXPECT_EQ(specifics.entry().events(0).completion().raw(), "Paris.");

  std::vector<mojom::AssociatedContentPtr> rebuilt_content;
  auto rebuilt = SpecificsToEntry(specifics, &rebuilt_content);
  ASSERT_TRUE(rebuilt);
  ASSERT_TRUE(rebuilt->events.has_value());
  ASSERT_EQ(rebuilt->events->size(), 1u);
  ASSERT_TRUE((*rebuilt->events)[0]->is_completion_event());
  EXPECT_EQ((*rebuilt->events)[0]->get_completion_event()->completion,
            "Paris.");
}

TEST(AIChatSyncConversionsTest, EntryRoundTripSearchQueriesEvent) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-queries";
  entry->created_time = base::Time::Now();
  entry->character_type = mojom::CharacterType::ASSISTANT;
  entry->action_type = mojom::ActionType::RESPONSE;

  std::vector<mojom::ConversationEntryEventPtr> events;
  auto sq = mojom::SearchQueriesEvent::New();
  sq->search_queries = {"capital of France", "Paris facts"};
  events.push_back(
      mojom::ConversationEntryEvent::NewSearchQueriesEvent(std::move(sq)));
  entry->events = std::move(events);

  auto specifics = EntryToSpecifics("conv-1", *entry, {});
  ASSERT_TRUE(specifics.entry().events(0).has_search_queries());
  EXPECT_EQ(specifics.entry().events(0).search_queries().queries_size(), 2);

  std::vector<mojom::AssociatedContentPtr> rebuilt_content;
  auto rebuilt = SpecificsToEntry(specifics, &rebuilt_content);
  ASSERT_TRUE(rebuilt);
  ASSERT_TRUE(rebuilt->events.has_value());
  ASSERT_TRUE((*rebuilt->events)[0]->is_search_queries_event());
  EXPECT_EQ((*rebuilt->events)[0]->get_search_queries_event()->search_queries,
            (std::vector<std::string>{"capital of France", "Paris facts"}));
}

TEST(AIChatSyncConversionsTest, EntryRoundTripWebSourcesEvent) {
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
  sources_event->rich_results = {"{\"x\":1}"};

  std::vector<mojom::ConversationEntryEventPtr> events;
  events.push_back(
      mojom::ConversationEntryEvent::NewSourcesEvent(std::move(sources_event)));
  entry->events = std::move(events);

  auto specifics = EntryToSpecifics("conv-1", *entry, {});

  std::vector<mojom::AssociatedContentPtr> rebuilt_content;
  auto rebuilt = SpecificsToEntry(specifics, &rebuilt_content);
  ASSERT_TRUE(rebuilt);
  ASSERT_TRUE(rebuilt->events.has_value());
  ASSERT_TRUE((*rebuilt->events)[0]->is_sources_event());
  const auto& rebuilt_sources = (*rebuilt->events)[0]->get_sources_event();
  ASSERT_EQ(rebuilt_sources->sources.size(), 1u);
  EXPECT_EQ(rebuilt_sources->sources[0]->title, "Paris");
  EXPECT_EQ(rebuilt_sources->sources[0]->url.spec(),
            "https://en.wikipedia.org/wiki/Paris");
  ASSERT_TRUE(rebuilt_sources->sources[0]->page_content.has_value());
  EXPECT_EQ(*rebuilt_sources->sources[0]->page_content,
            "Paris is the capital of France.");
  ASSERT_TRUE(rebuilt_sources->sources[0]->extra_snippets.has_value());
  EXPECT_EQ(rebuilt_sources->sources[0]->extra_snippets->size(), 2u);
  EXPECT_EQ(rebuilt_sources->rich_results,
            (std::vector<std::string>{"{\"x\":1}"}));
}

TEST(AIChatSyncConversionsTest, EntryRoundTripToolUseEvent) {
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

  std::vector<mojom::AssociatedContentPtr> rebuilt_content;
  auto rebuilt = SpecificsToEntry(specifics, &rebuilt_content);
  ASSERT_TRUE(rebuilt);
  ASSERT_TRUE(rebuilt->events.has_value());
  ASSERT_TRUE((*rebuilt->events)[0]->is_tool_use_event());
  const auto& rebuilt_tool = (*rebuilt->events)[0]->get_tool_use_event();
  EXPECT_EQ(rebuilt_tool->tool_name, "search");
  EXPECT_EQ(rebuilt_tool->id, "tool-1");
  EXPECT_EQ(rebuilt_tool->arguments_json, "{\"q\":\"test\"}");
  EXPECT_TRUE(rebuilt_tool->is_server_result);
  ASSERT_TRUE(rebuilt_tool->output.has_value());
  ASSERT_EQ(rebuilt_tool->output->size(), 1u);
  ASSERT_TRUE((*rebuilt_tool->output)[0]->is_text_content_block());
  EXPECT_EQ((*rebuilt_tool->output)[0]->get_text_content_block()->text,
            "result");
}

TEST(AIChatSyncConversionsTest, EntryAssociatedContentFilteredByEntry) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-with-content";
  entry->created_time = base::Time::Now();
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;

  std::vector<mojom::AssociatedContentPtr> all_content;
  auto mine = mojom::AssociatedContent::New();
  mine->uuid = "content-mine";
  mine->title = "Mine";
  mine->url = GURL("https://example.com/mine");
  mine->content_type = mojom::ContentType::PageContent;
  mine->content_used_percentage = 75;
  mine->conversation_turn_uuid = "entry-with-content";
  all_content.push_back(std::move(mine));

  auto other = mojom::AssociatedContent::New();
  other->uuid = "content-other";
  other->title = "Other";
  other->url = GURL("https://example.com/other");
  other->content_type = mojom::ContentType::PageContent;
  other->content_used_percentage = 50;
  other->conversation_turn_uuid = "different-entry";
  all_content.push_back(std::move(other));

  auto specifics = EntryToSpecifics("conv-1", *entry, all_content);
  ASSERT_EQ(specifics.entry().associated_content_size(), 1);
  EXPECT_EQ(specifics.entry().associated_content(0).uuid(), "content-mine");

  std::vector<mojom::AssociatedContentPtr> rebuilt_content;
  auto rebuilt = SpecificsToEntry(specifics, &rebuilt_content);
  ASSERT_TRUE(rebuilt);
  ASSERT_EQ(rebuilt_content.size(), 1u);
  EXPECT_EQ(rebuilt_content[0]->uuid, "content-mine");
  EXPECT_EQ(rebuilt_content[0]->title, "Mine");
  EXPECT_EQ(rebuilt_content[0]->url.spec(), "https://example.com/mine");
  EXPECT_EQ(rebuilt_content[0]->content_used_percentage, 75);
  ASSERT_TRUE(rebuilt_content[0]->conversation_turn_uuid.has_value());
  EXPECT_EQ(*rebuilt_content[0]->conversation_turn_uuid, "entry-with-content");
}

TEST(AIChatSyncConversionsTest,
     SpecificsToConversationReturnsNullForWrongKind) {
  sync_pb::AIChatConversationSpecifics specifics;
  specifics.mutable_entry()->set_uuid("e1");
  EXPECT_FALSE(SpecificsToConversationMetadata(specifics));
}

TEST(AIChatSyncConversionsTest, SpecificsToEntryReturnsNullForWrongKind) {
  sync_pb::AIChatConversationSpecifics specifics;
  specifics.mutable_conversation()->set_uuid("c1");
  std::vector<mojom::AssociatedContentPtr> content;
  EXPECT_FALSE(SpecificsToEntry(specifics, &content));
}

TEST(AIChatSyncConversionsTest, EntryRoundTripUploadedFiles) {
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

  std::vector<mojom::AssociatedContentPtr> rebuilt_content;
  auto rebuilt = SpecificsToEntry(specifics, &rebuilt_content);
  ASSERT_TRUE(rebuilt);
  ASSERT_TRUE(rebuilt->uploaded_files.has_value());
  ASSERT_EQ(rebuilt->uploaded_files->size(), 2u);
  EXPECT_EQ((*rebuilt->uploaded_files)[0]->filename, "cat.jpg");
  EXPECT_EQ((*rebuilt->uploaded_files)[0]->data.size(), 5u);
  ASSERT_TRUE((*rebuilt->uploaded_files)[1]->extracted_text.has_value());
  EXPECT_EQ((*rebuilt->uploaded_files)[1]->extracted_text->size(), 2048u);
}

TEST(AIChatSyncConversionsTest, EntryRoundTripWebSourcesContentBlock) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-tool-ws";
  entry->created_time = base::Time::Now();
  entry->character_type = mojom::CharacterType::ASSISTANT;
  entry->action_type = mojom::ActionType::RESPONSE;

  auto ws_block = mojom::WebSourcesContentBlock::New();
  auto source = mojom::WebSource::New();
  source->title = "MDN";
  source->url = GURL("https://developer.mozilla.org/");
  source->page_content = "Web docs.";
  ws_block->sources.push_back(std::move(source));
  ws_block->queries = {"mdn web docs"};
  ws_block->rich_results = {"{\"a\":1}"};

  std::vector<mojom::ContentBlockPtr> output;
  output.push_back(
      mojom::ContentBlock::NewWebSourcesContentBlock(std::move(ws_block)));

  auto tool_use = mojom::ToolUseEvent::New();
  tool_use->tool_name = "search";
  tool_use->id = "tool-2";
  tool_use->arguments_json = "{}";
  tool_use->output = std::move(output);

  std::vector<mojom::ConversationEntryEventPtr> events;
  events.push_back(
      mojom::ConversationEntryEvent::NewToolUseEvent(std::move(tool_use)));
  entry->events = std::move(events);

  auto specifics = EntryToSpecifics("conv-1", *entry, {});
  ASSERT_EQ(specifics.entry().events(0).tool_use().output_size(), 1);
  EXPECT_TRUE(specifics.entry()
                  .events(0)
                  .tool_use()
                  .output(0)
                  .has_web_sources_content_block());

  std::vector<mojom::AssociatedContentPtr> rebuilt_content;
  auto rebuilt = SpecificsToEntry(specifics, &rebuilt_content);
  ASSERT_TRUE(rebuilt);
  const auto& rebuilt_tool = (*rebuilt->events)[0]->get_tool_use_event();
  ASSERT_TRUE(rebuilt_tool->output.has_value());
  ASSERT_EQ(rebuilt_tool->output->size(), 1u);
  ASSERT_TRUE((*rebuilt_tool->output)[0]->is_web_sources_content_block());
  const auto& rebuilt_block =
      (*rebuilt_tool->output)[0]->get_web_sources_content_block();
  ASSERT_EQ(rebuilt_block->sources.size(), 1u);
  EXPECT_EQ(rebuilt_block->sources[0]->title, "MDN");
  ASSERT_TRUE(rebuilt_block->sources[0]->page_content.has_value());
  EXPECT_EQ(*rebuilt_block->sources[0]->page_content, "Web docs.");
  EXPECT_EQ(rebuilt_block->queries, (std::vector<std::string>{"mdn web docs"}));
  EXPECT_EQ(rebuilt_block->rich_results,
            (std::vector<std::string>{"{\"a\":1}"}));
}

TEST(AIChatSyncConversionsTest, EntryAssociatedContentTextRoundTrip) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-ac";
  entry->created_time = base::Time::Now();
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;

  std::vector<mojom::AssociatedContentPtr> all_content;
  auto ac = mojom::AssociatedContent::New();
  ac->uuid = "ac-1";
  ac->title = "Page";
  ac->url = GURL("https://example.com/");
  ac->content_type = mojom::ContentType::PageContent;
  ac->content_used_percentage = 100;
  ac->conversation_turn_uuid = "entry-ac";
  all_content.push_back(std::move(ac));

  base::flat_map<std::string, std::string> texts;
  // Highly-compressible text so gzip kicks in (validates the encoding path).
  texts["ac-1"] = std::string(4096, 'P');

  auto specifics = EntryToSpecifics("conv-1", *entry, all_content, texts);
  ASSERT_EQ(specifics.entry().associated_content_size(), 1);
  ASSERT_TRUE(specifics.entry().associated_content(0).has_last_contents());
  EXPECT_TRUE(
      specifics.entry().associated_content(0).last_contents().has_gzipped());

  std::vector<mojom::AssociatedContentPtr> rebuilt_content;
  base::flat_map<std::string, std::string> rebuilt_texts;
  auto rebuilt = SpecificsToEntry(specifics, &rebuilt_content, &rebuilt_texts);
  ASSERT_TRUE(rebuilt);
  ASSERT_EQ(rebuilt_content.size(), 1u);
  ASSERT_TRUE(rebuilt_texts.contains("ac-1"));
  EXPECT_EQ(rebuilt_texts["ac-1"], std::string(4096, 'P'));
}

TEST(AIChatSyncConversionsTest, EntryRoundTripSkillAndNearVerification) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-skill";
  entry->created_time =
      base::Time::FromDeltaSinceWindowsEpoch(base::Microseconds(7));
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;
  entry->skill = mojom::SkillEntry::New("/summarize", "Summarize this page");
  entry->near_verification_status = mojom::NEARVerificationStatus::New(true);

  auto specifics = EntryToSpecifics("conv-1", *entry, {});
  std::vector<mojom::AssociatedContentPtr> rebuilt_content;
  auto rebuilt = SpecificsToEntry(specifics, &rebuilt_content);
  ASSERT_TRUE(rebuilt);
  ASSERT_TRUE(rebuilt->skill);
  EXPECT_EQ(rebuilt->skill->shortcut, "/summarize");
  EXPECT_EQ(rebuilt->skill->prompt, "Summarize this page");
  ASSERT_TRUE(rebuilt->near_verification_status);
  EXPECT_TRUE(rebuilt->near_verification_status->verified);
}

TEST(AIChatSyncConversionsTest, EntryRoundTripWithoutSkillOrNear) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-plain";
  entry->created_time =
      base::Time::FromDeltaSinceWindowsEpoch(base::Microseconds(7));
  entry->character_type = mojom::CharacterType::HUMAN;
  entry->action_type = mojom::ActionType::QUERY;

  auto specifics = EntryToSpecifics("conv-1", *entry, {});
  std::vector<mojom::AssociatedContentPtr> rebuilt_content;
  auto rebuilt = SpecificsToEntry(specifics, &rebuilt_content);
  ASSERT_TRUE(rebuilt);
  EXPECT_FALSE(rebuilt->skill);
  EXPECT_FALSE(rebuilt->near_verification_status);
}

TEST(AIChatSyncConversionsTest, FitEntryNoOpBelowBudget) {
  sync_pb::AIChatConversationSpecifics_Entry entry;
  entry.set_uuid("e1");
  entry.set_conversation_uuid("c1");
  entry.set_entry_text("short");

  ASSERT_LE(entry.ByteSizeLong(), kSyncMaxRecordBytes);
  EXPECT_TRUE(FitEntryWithinSyncBudget(&entry));
  // Nothing should have been touched.
  EXPECT_EQ(entry.entry_text(), "short");
}

TEST(AIChatSyncConversionsTest, FitEntryOmitsFileBytesFirst) {
  sync_pb::AIChatConversationSpecifics_Entry entry;
  entry.set_uuid("e1");
  entry.set_conversation_uuid("c1");
  // Make a single uploaded file that on its own exceeds the budget.
  auto* file = entry.add_uploaded_files();
  file->set_filename("big.bin");
  file->set_filesize(kSyncMaxRecordBytes + 1024);
  const std::string file_bytes(kSyncMaxRecordBytes + 1024, '\x01');
  file->set_data(file_bytes);

  // Add an AC with a small last_contents so we can verify it is NOT omitted
  // when the file alone is enough to bring us under budget.
  auto* ac = entry.add_associated_content();
  ac->set_uuid("ac-1");
  WriteCompressibleString("small page text", ac->mutable_last_contents());

  ASSERT_GT(entry.ByteSizeLong(), kSyncMaxRecordBytes);
  EXPECT_TRUE(FitEntryWithinSyncBudget(&entry));
  EXPECT_FALSE(entry.uploaded_files(0).has_data());
  // The omitted bytes leave behind a hash of the original content.
  EXPECT_EQ(entry.uploaded_files(0).omitted_data_hash(),
            base::PersistentHash(file_bytes));
  // The AC's last_contents must still be intact.
  EXPECT_FALSE(
      entry.associated_content(0).last_contents().has_omitted_content_hash());
  EXPECT_LE(entry.ByteSizeLong(), kSyncMaxRecordBytes);
}

TEST(AIChatSyncConversionsTest, FitEntryOmitsCategoriesInOrder) {
  // Build an entry that requires more than one category to be omitted. The
  // file bytes step (1) alone is not enough; the AC last_contents (2) must
  // also be omitted. Use raw (non-gzipped) bytes for the AC text so the
  // serialized size matches the input size.
  sync_pb::AIChatConversationSpecifics_Entry entry;
  entry.set_uuid("e1");
  entry.set_conversation_uuid("c1");

  // 50 KB of file bytes (step 1).
  auto* file = entry.add_uploaded_files();
  file->set_data(std::string(50 * 1024, '\x02'));

  // 400 KB of raw AC last_contents (step 2) — bypass WriteCompressibleString
  // so we get a deterministic on-the-wire size that survives step 1.
  auto* ac = entry.add_associated_content();
  ac->set_uuid("ac-1");
  ac->mutable_last_contents()->set_raw(std::string(400 * 1024, 'A'));

  ASSERT_GT(entry.ByteSizeLong(), kSyncMaxRecordBytes);
  EXPECT_TRUE(FitEntryWithinSyncBudget(&entry));
  // Step 1: file bytes were omitted.
  EXPECT_TRUE(entry.uploaded_files(0).has_omitted_data_hash());
  // Step 2: AC last_contents was omitted (file alone wasn't enough).
  EXPECT_TRUE(
      entry.associated_content(0).last_contents().has_omitted_content_hash());
  EXPECT_LE(entry.ByteSizeLong(), kSyncMaxRecordBytes);
}

TEST(AIChatSyncConversionsTest, FitEntryRefusesPathologicalEntry) {
  // Pile on a field that's NOT in any omission step so the size-budget policy
  // cannot shrink it. `selected_text` is currently a plain string and not in
  // the priority list — exactly what we want to force the refusal path.
  sync_pb::AIChatConversationSpecifics_Entry entry;
  entry.set_uuid("pathological");
  entry.set_conversation_uuid("c1");
  entry.set_selected_text(std::string(kSyncMaxRecordBytes + 1024, 'Z'));

  ASSERT_GT(entry.ByteSizeLong(), kSyncMaxRecordBytes);
  EXPECT_FALSE(FitEntryWithinSyncBudget(&entry));
  // No mutations to omittable fields (there are none on this entry).
}

}  // namespace ai_chat
