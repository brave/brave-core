// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"

#include <string>
#include <utility>
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

TEST(AIChatSyncConversionsTest, ConversationMetadataRoundTrip) {
  auto original = mojom::Conversation::New();
  original->uuid = "conv-123";
  original->title = "Test conversation";
  original->model_key = "claude-opus";
  original->total_tokens = 4096;
  original->trimmed_tokens = 256;
  original->updated_time = base::Time::Now();

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
  // Allow microsecond rounding tolerance.
  EXPECT_LE((rebuilt->updated_time - original->updated_time).magnitude(),
            base::Microseconds(1));
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

TEST(AIChatSyncConversionsTest, StorageKeyPrefixes) {
  sync_pb::AIChatConversationSpecifics conv_specifics;
  conv_specifics.mutable_conversation()->set_uuid("abc");
  EXPECT_EQ(GetStorageKeyFromSpecifics(conv_specifics), "c:abc");
  EXPECT_EQ(GetClientTagFromSpecifics(conv_specifics), "c:abc");

  sync_pb::AIChatConversationSpecifics entry_specifics;
  entry_specifics.mutable_entry()->set_uuid("def");
  entry_specifics.mutable_entry()->set_conversation_uuid("abc");
  EXPECT_EQ(GetStorageKeyFromSpecifics(entry_specifics), "e:def");
  EXPECT_EQ(GetClientTagFromSpecifics(entry_specifics), "e:def");

  sync_pb::AIChatConversationSpecifics empty;
  EXPECT_TRUE(GetStorageKeyFromSpecifics(empty).empty());
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

TEST(AIChatSyncConversionsTest, CompressibleStringSmallBypassesGzip) {
  sync_pb::AIChatCompressibleString out;
  WriteCompressibleString("hello", &out);
  EXPECT_TRUE(out.has_raw());
  EXPECT_FALSE(out.has_gzipped());
  EXPECT_EQ(out.raw(), "hello");
  auto round = ReadCompressibleString(out);
  ASSERT_TRUE(round);
  EXPECT_EQ(*round, "hello");
}

TEST(AIChatSyncConversionsTest, CompressibleStringLargeCompresses) {
  // Highly compressible input (long repetition) so gzip is guaranteed to
  // shrink it under the input size.
  std::string original(8192, 'A');
  sync_pb::AIChatCompressibleString out;
  WriteCompressibleString(original, &out);
  EXPECT_FALSE(out.has_raw());
  ASSERT_TRUE(out.has_gzipped());
  EXPECT_LT(out.gzipped().size(), original.size());
  auto round = ReadCompressibleString(out);
  ASSERT_TRUE(round);
  EXPECT_EQ(*round, original);
}

TEST(AIChatSyncConversionsTest, CompressibleStringTruncationSentinel) {
  sync_pb::AIChatCompressibleString out;
  WriteCompressibleString("payload", &out);
  ASSERT_TRUE(out.has_raw());

  MarkCompressibleStringTruncated(&out);
  EXPECT_TRUE(out.was_truncated_for_sync());
  EXPECT_FALSE(out.has_raw());
  EXPECT_FALSE(out.has_gzipped());

  EXPECT_FALSE(ReadCompressibleString(out));
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
  EXPECT_FALSE(specifics.entry().uploaded_files(0).data_truncated_for_sync());
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

TEST(AIChatSyncConversionsTest, EntryRoundTripPermissionChallenge) {
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = "entry-tool";
  entry->created_time = base::Time::Now();
  entry->character_type = mojom::CharacterType::ASSISTANT;
  entry->action_type = mojom::ActionType::RESPONSE;

  auto tool_use = mojom::ToolUseEvent::New();
  tool_use->tool_name = "shell";
  tool_use->id = "tool-1";
  tool_use->arguments_json = "{}";
  tool_use->permission_challenge = mojom::PermissionChallenge::New();
  tool_use->permission_challenge->assessment = "Server says no";
  tool_use->permission_challenge->plan = "Run rm -rf /";

  std::vector<mojom::ConversationEntryEventPtr> events;
  events.push_back(
      mojom::ConversationEntryEvent::NewToolUseEvent(std::move(tool_use)));
  entry->events = std::move(events);

  auto specifics = EntryToSpecifics("conv-1", *entry, {});
  ASSERT_TRUE(
      specifics.entry().events(0).tool_use().has_permission_challenge());
  EXPECT_EQ(specifics.entry()
                .events(0)
                .tool_use()
                .permission_challenge()
                .assessment(),
            "Server says no");

  std::vector<mojom::AssociatedContentPtr> rebuilt_content;
  auto rebuilt = SpecificsToEntry(specifics, &rebuilt_content);
  ASSERT_TRUE(rebuilt);
  const auto& rebuilt_tool = (*rebuilt->events)[0]->get_tool_use_event();
  ASSERT_TRUE(rebuilt_tool->permission_challenge);
  ASSERT_TRUE(rebuilt_tool->permission_challenge->assessment.has_value());
  EXPECT_EQ(*rebuilt_tool->permission_challenge->assessment, "Server says no");
  ASSERT_TRUE(rebuilt_tool->permission_challenge->plan.has_value());
  EXPECT_EQ(*rebuilt_tool->permission_challenge->plan, "Run rm -rf /");
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

TEST(AIChatSyncConversionsTest, TruncationNoOpBelowThreshold) {
  sync_pb::AIChatConversationSpecifics_Entry entry;
  entry.set_uuid("e1");
  entry.set_conversation_uuid("c1");
  entry.set_entry_text("short");

  ASSERT_LE(entry.ByteSizeLong(), kSyncMaxRecordBytes);
  EXPECT_TRUE(TruncateEntryForSync(&entry));
  // Nothing should have been touched.
  EXPECT_EQ(entry.entry_text(), "short");
}

TEST(AIChatSyncConversionsTest, TruncationDropsFileBytesFirst) {
  sync_pb::AIChatConversationSpecifics_Entry entry;
  entry.set_uuid("e1");
  entry.set_conversation_uuid("c1");
  // Make a single uploaded file that on its own exceeds the budget.
  auto* file = entry.add_uploaded_files();
  file->set_filename("big.bin");
  file->set_filesize(kSyncMaxRecordBytes + 1024);
  file->set_data(std::string(kSyncMaxRecordBytes + 1024, '\x01'));

  // Add an AC with a small last_contents so we can verify it is NOT
  // truncated when the file alone is enough to bring us under budget.
  auto* ac = entry.add_associated_content();
  ac->set_uuid("ac-1");
  WriteCompressibleString("small page text", ac->mutable_last_contents());

  ASSERT_GT(entry.ByteSizeLong(), kSyncMaxRecordBytes);
  EXPECT_TRUE(TruncateEntryForSync(&entry));
  EXPECT_TRUE(entry.uploaded_files(0).data_truncated_for_sync());
  EXPECT_FALSE(entry.uploaded_files(0).has_data() &&
               !entry.uploaded_files(0).data().empty());
  // The AC's last_contents must still be intact.
  EXPECT_FALSE(
      entry.associated_content(0).last_contents().was_truncated_for_sync());
  EXPECT_LE(entry.ByteSizeLong(), kSyncMaxRecordBytes);
}

TEST(AIChatSyncConversionsTest, TruncationFallsThroughCategoriesInOrder) {
  // Build an entry that requires more than one category to be dropped. The
  // file bytes step (1) alone is not enough; the AC last_contents (2) must
  // also be dropped. Use raw (non-gzipped) bytes for the AC text so the
  // serialized size matches the input size.
  sync_pb::AIChatConversationSpecifics_Entry entry;
  entry.set_uuid("e1");
  entry.set_conversation_uuid("c1");

  // 50 KB of file bytes (truncation step 1).
  auto* file = entry.add_uploaded_files();
  file->set_data(std::string(50 * 1024, '\x02'));

  // 400 KB of raw AC last_contents (step 2) — bypass WriteCompressibleString
  // so we get a deterministic on-the-wire size that survives step 1.
  auto* ac = entry.add_associated_content();
  ac->set_uuid("ac-1");
  ac->mutable_last_contents()->set_raw(std::string(400 * 1024, 'A'));

  ASSERT_GT(entry.ByteSizeLong(), kSyncMaxRecordBytes);
  EXPECT_TRUE(TruncateEntryForSync(&entry));
  // Step 1: file bytes were dropped.
  EXPECT_TRUE(entry.uploaded_files(0).data_truncated_for_sync());
  // Step 2: AC last_contents was dropped (file alone wasn't enough).
  EXPECT_TRUE(
      entry.associated_content(0).last_contents().was_truncated_for_sync());
  EXPECT_LE(entry.ByteSizeLong(), kSyncMaxRecordBytes);
}

TEST(AIChatSyncConversionsTest, TruncationRefusesPathologicalEntry) {
  // Pile on a field that's NOT in any truncation step so truncation cannot
  // shrink it. `selected_text` is currently a plain string and not in the
  // priority list — exactly what we want to force the refusal path.
  sync_pb::AIChatConversationSpecifics_Entry entry;
  entry.set_uuid("pathological");
  entry.set_conversation_uuid("c1");
  entry.set_selected_text(std::string(kSyncMaxRecordBytes + 1024, 'Z'));

  ASSERT_GT(entry.ByteSizeLong(), kSyncMaxRecordBytes);
  EXPECT_FALSE(TruncateEntryForSync(&entry));
  // No mutations to truncatable fields (there are none on this entry).
}

}  // namespace
}  // namespace ai_chat
