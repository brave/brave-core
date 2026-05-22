/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/time/time.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/sync/protocol/ai_chat_specifics.pb.h"
#include "components/sync/protocol/entity_data.h"
#include "components/sync/protocol/entity_specifics.pb.h"
#include "third_party/zlib/google/compression_utils.h"
#include "url/gurl.h"

namespace ai_chat {

void WriteCompressibleString(std::string_view value,
                             sync_pb::AIChatCompressibleString* out) {
  // Writing a real value supersedes any previous truncation sentinel on
  // |out| (the merge path reuses an existing AIChatCompressibleString that
  // may have been marked truncated by the sender).
  out->clear_was_truncated_for_sync();
  if (value.size() < kSyncCompressionThresholdBytes) {
    out->set_raw(std::string(value));
    return;
  }
  std::string compressed;
  if (compression::GzipCompress(value, &compressed) &&
      compressed.size() < value.size()) {
    out->set_gzipped(std::move(compressed));
  } else {
    out->set_raw(std::string(value));
  }
}

void MarkCompressibleStringTruncated(sync_pb::AIChatCompressibleString* out) {
  out->clear_value();
  out->set_was_truncated_for_sync(true);
}

std::optional<std::string> ReadCompressibleString(
    const sync_pb::AIChatCompressibleString& in) {
  if (in.was_truncated_for_sync()) {
    return std::nullopt;
  }
  if (in.has_gzipped()) {
    std::string out;
    if (!compression::GzipUncompress(in.gzipped(), &out)) {
      return std::nullopt;
    }
    return out;
  }
  if (in.has_raw()) {
    return in.raw();
  }
  return std::nullopt;
}

bool TruncateEntryForSync(sync_pb::AIChatConversationSpecifics::Entry* entry) {
  if (entry->ByteSizeLong() <= kSyncMaxRecordBytes) {
    return true;
  }

  // Step 1: drop raw uploaded file bytes (least re-derivable, but already
  // unhelpful on the receiver without the local file present).
  for (auto& file : *entry->mutable_uploaded_files()) {
    if (file.has_data() && !file.data().empty()) {
      file.clear_data();
      file.set_data_truncated_for_sync(true);
    }
  }
  if (entry->ByteSizeLong() <= kSyncMaxRecordBytes) {
    return true;
  }

  // Step 2: associated_content.last_contents — the full page text. Most
  // re-derivable since the receiver still has the URL.
  for (auto& ac : *entry->mutable_associated_content()) {
    if (ac.has_last_contents() &&
        !ac.last_contents().was_truncated_for_sync()) {
      MarkCompressibleStringTruncated(ac.mutable_last_contents());
    }
  }
  if (entry->ByteSizeLong() <= kSyncMaxRecordBytes) {
    return true;
  }

  // Step 3: uploaded_files.extracted_text — derivable from the file bytes
  // (which may already have been dropped, but that's fine).
  for (auto& file : *entry->mutable_uploaded_files()) {
    if (file.has_extracted_text() &&
        !file.extracted_text().was_truncated_for_sync()) {
      MarkCompressibleStringTruncated(file.mutable_extracted_text());
    }
  }
  if (entry->ByteSizeLong() <= kSyncMaxRecordBytes) {
    return true;
  }

  // Step 4: web_sources.page_content (search-result page snippets).
  for (auto& event : *entry->mutable_events()) {
    if (!event.has_web_sources()) {
      continue;
    }
    for (auto& src : *event.mutable_web_sources()->mutable_sources()) {
      if (src.has_page_content() &&
          !src.page_content().was_truncated_for_sync()) {
        MarkCompressibleStringTruncated(src.mutable_page_content());
      }
    }
  }
  if (entry->ByteSizeLong() <= kSyncMaxRecordBytes) {
    return true;
  }

  // Step 5: tool_use.output text content block text.
  for (auto& event : *entry->mutable_events()) {
    if (!event.has_tool_use()) {
      continue;
    }
    for (auto& block : *event.mutable_tool_use()->mutable_output()) {
      if (!block.has_text_content_block()) {
        continue;
      }
      auto* text = block.mutable_text_content_block()->mutable_text();
      if (!text->was_truncated_for_sync()) {
        MarkCompressibleStringTruncated(text);
      }
    }
  }
  if (entry->ByteSizeLong() <= kSyncMaxRecordBytes) {
    return true;
  }

  // Step 6: tool_use.arguments_json.
  for (auto& event : *entry->mutable_events()) {
    if (event.has_tool_use() && event.tool_use().has_arguments_json() &&
        !event.tool_use().arguments_json().was_truncated_for_sync()) {
      MarkCompressibleStringTruncated(
          event.mutable_tool_use()->mutable_arguments_json());
    }
  }
  if (entry->ByteSizeLong() <= kSyncMaxRecordBytes) {
    return true;
  }

  // Step 7: web_sources.rich_results (JSON-formatted SERP payloads).
  for (auto& event : *entry->mutable_events()) {
    if (!event.has_web_sources()) {
      continue;
    }
    for (auto& rr : *event.mutable_web_sources()->mutable_rich_results()) {
      if (!rr.was_truncated_for_sync()) {
        MarkCompressibleStringTruncated(&rr);
      }
    }
  }
  if (entry->ByteSizeLong() <= kSyncMaxRecordBytes) {
    return true;
  }

  // Step 8: completion text.
  for (auto& event : *entry->mutable_events()) {
    if (event.has_completion() &&
        !event.completion().was_truncated_for_sync()) {
      MarkCompressibleStringTruncated(event.mutable_completion());
    }
  }
  if (entry->ByteSizeLong() <= kSyncMaxRecordBytes) {
    return true;
  }

  // Pathological: every truncatable field has been dropped and the record
  // is still over budget. Refuse the commit.
  DLOG(ERROR) << "AI Chat entry " << entry->uuid()
              << " exceeds the sync record budget after full truncation; "
              << "refusing to commit. Size=" << entry->ByteSizeLong();
  return false;
}

namespace {

// --- Upload helpers: mojom → sync proto ---

void WriteAssociatedContent(const mojom::AssociatedContent& content,
                            std::optional<std::string_view> last_contents,
                            sync_pb::AIChatAssociatedContentProto* proto) {
  proto->set_uuid(content.uuid);
  proto->set_title(content.title);
  if (content.url.is_valid()) {
    proto->set_url(content.url.spec());
  }
  proto->set_content_type(static_cast<int32_t>(content.content_type));
  proto->set_content_used_percentage(content.content_used_percentage);
  // When |last_contents| is absent the receiver preserves any local text for
  // this UUID; we do not need to set the field at all.
  if (last_contents) {
    WriteCompressibleString(*last_contents, proto->mutable_last_contents());
  }
}

void WriteWebSource(const mojom::WebSource& source,
                    sync_pb::AIChatWebSource* proto) {
  proto->set_title(source.title);
  if (source.url.is_valid()) {
    proto->set_url(source.url.spec());
  }
  if (source.favicon_url.is_valid()) {
    proto->set_favicon_url(source.favicon_url.spec());
  }
  if (source.page_content) {
    WriteCompressibleString(*source.page_content,
                            proto->mutable_page_content());
  }
  if (source.extra_snippets) {
    for (const auto& snippet : *source.extra_snippets) {
      proto->add_extra_snippets(snippet);
    }
  }
}

void WriteWebSourcesContentBlock(const mojom::WebSourcesContentBlock& block,
                                 sync_pb::AIChatWebSourcesContentBlock* proto) {
  for (const auto& source : block.sources) {
    WriteWebSource(*source, proto->add_sources());
  }
  for (const auto& q : block.queries) {
    proto->add_queries(q);
  }
  for (const auto& rr : block.rich_results) {
    WriteCompressibleString(rr, proto->add_rich_results());
  }
}

void WriteToolUse(const mojom::ToolUseEvent& tool_use,
                  sync_pb::AIChatToolUseEvent* proto) {
  proto->set_tool_name(tool_use.tool_name);
  proto->set_id(tool_use.id);
  WriteCompressibleString(tool_use.arguments_json,
                          proto->mutable_arguments_json());
  proto->set_is_server_result(tool_use.is_server_result);
  if (tool_use.output) {
    for (const auto& block : *tool_use.output) {
      auto* block_proto = proto->add_output();
      if (block->is_text_content_block()) {
        WriteCompressibleString(
            block->get_text_content_block()->text,
            block_proto->mutable_text_content_block()->mutable_text());
      } else if (block->is_image_content_block()) {
        block_proto->mutable_image_content_block()->set_image_url(
            block->get_image_content_block()->image_url.spec());
      } else if (block->is_web_sources_content_block()) {
        WriteWebSourcesContentBlock(
            *block->get_web_sources_content_block(),
            block_proto->mutable_web_sources_content_block());
      }
      // Other ContentBlock variants are runtime-only and intentionally not
      // synced (see ai_chat_specifics.proto).
    }
  }
  if (tool_use.artifacts) {
    for (const auto& artifact : *tool_use.artifacts) {
      auto* a = proto->add_artifacts();
      a->set_type(artifact->type);
      WriteCompressibleString(artifact->content_json,
                              a->mutable_content_json());
    }
  }
  if (tool_use.permission_challenge) {
    auto* pc = proto->mutable_permission_challenge();
    if (tool_use.permission_challenge->assessment) {
      pc->set_assessment(*tool_use.permission_challenge->assessment);
    }
    if (tool_use.permission_challenge->plan) {
      pc->set_plan(*tool_use.permission_challenge->plan);
    }
  }
}

void WriteUploadedFile(const mojom::UploadedFile& file,
                       sync_pb::AIChatUploadedFile* proto) {
  proto->set_filename(file.filename);
  proto->set_filesize(file.filesize);
  proto->set_type(static_cast<int32_t>(file.type));
  // Raw bytes are inlined here; the truncation policy may later drop them
  // and set |data_truncated_for_sync| if the entry exceeds the size cap.
  if (!file.data.empty()) {
    proto->set_data(file.data.data(), file.data.size());
  }
  if (file.extracted_text) {
    WriteCompressibleString(*file.extracted_text,
                            proto->mutable_extracted_text());
  }
}

void WriteEntryFields(const mojom::ConversationTurn& entry,
                      sync_pb::AIChatConversationSpecifics::Entry* proto) {
  if (entry.uuid) {
    proto->set_uuid(*entry.uuid);
  }
  proto->set_date_unix_epoch_micros(
      entry.created_time.ToDeltaSinceWindowsEpoch().InMicroseconds());
  proto->set_entry_text(entry.text);
  if (entry.prompt) {
    proto->set_prompt(*entry.prompt);
  }
  proto->set_character_type(static_cast<int32_t>(entry.character_type));
  proto->set_action_type(static_cast<int32_t>(entry.action_type));
  if (entry.selected_text) {
    proto->set_selected_text(*entry.selected_text);
  }
  if (entry.model_key) {
    proto->set_model_key(*entry.model_key);
  }

  if (entry.events) {
    int order = 0;
    for (const auto& event : *entry.events) {
      auto* event_proto = proto->add_events();
      event_proto->set_event_order(order++);
      if (event->is_completion_event()) {
        WriteCompressibleString(event->get_completion_event()->completion,
                                event_proto->mutable_completion());
      } else if (event->is_search_queries_event()) {
        const auto& queries = event->get_search_queries_event()->search_queries;
        auto* sq = event_proto->mutable_search_queries();
        for (const auto& q : queries) {
          sq->add_queries(q);
        }
      } else if (event->is_sources_event()) {
        auto* ws = event_proto->mutable_web_sources();
        const auto& sources_event = event->get_sources_event();
        for (const auto& source : sources_event->sources) {
          WriteWebSource(*source, ws->add_sources());
        }
        for (const auto& rr : sources_event->rich_results) {
          WriteCompressibleString(rr, ws->add_rich_results());
        }
      } else if (event->is_inline_search_event()) {
        auto* is = event_proto->mutable_inline_search();
        is->set_query(event->get_inline_search_event()->query);
        WriteCompressibleString(event->get_inline_search_event()->results_json,
                                is->mutable_results_json());
      } else if (event->is_tool_use_event()) {
        WriteToolUse(*event->get_tool_use_event(),
                     event_proto->mutable_tool_use());
      }
    }
  }

  if (entry.uploaded_files) {
    for (const auto& file : *entry.uploaded_files) {
      WriteUploadedFile(*file, proto->add_uploaded_files());
    }
  }
}

// --- Download helpers: sync proto → mojom ---

mojom::WebSourcePtr ProtoToWebSource(const sync_pb::AIChatWebSource& proto) {
  auto source = mojom::WebSource::New();
  source->title = proto.title();
  if (proto.has_url()) {
    source->url = GURL(proto.url());
  }
  if (proto.has_favicon_url()) {
    source->favicon_url = GURL(proto.favicon_url());
  }
  if (proto.has_page_content()) {
    if (auto pc = ReadCompressibleString(proto.page_content())) {
      source->page_content = std::move(*pc);
    }
  }
  if (proto.extra_snippets_size() > 0) {
    std::vector<std::string> snippets;
    snippets.reserve(proto.extra_snippets_size());
    for (const auto& snippet : proto.extra_snippets()) {
      snippets.push_back(snippet);
    }
    source->extra_snippets = std::move(snippets);
  }
  return source;
}

mojom::WebSourcesContentBlockPtr ProtoToWebSourcesContentBlock(
    const sync_pb::AIChatWebSourcesContentBlock& proto) {
  auto block = mojom::WebSourcesContentBlock::New();
  for (const auto& source : proto.sources()) {
    block->sources.push_back(ProtoToWebSource(source));
  }
  for (const auto& q : proto.queries()) {
    block->queries.push_back(q);
  }
  for (const auto& rr : proto.rich_results()) {
    if (auto value = ReadCompressibleString(rr)) {
      block->rich_results.push_back(std::move(*value));
    }
  }
  return block;
}

mojom::ToolUseEventPtr ProtoToToolUse(
    const sync_pb::AIChatToolUseEvent& proto) {
  auto tool_use = mojom::ToolUseEvent::New();
  tool_use->tool_name = proto.tool_name();
  tool_use->id = proto.id();
  if (proto.has_arguments_json()) {
    if (auto args = ReadCompressibleString(proto.arguments_json())) {
      tool_use->arguments_json = std::move(*args);
    }
  }
  tool_use->is_server_result = proto.is_server_result();
  if (proto.output_size() > 0) {
    std::vector<mojom::ContentBlockPtr> blocks;
    blocks.reserve(proto.output_size());
    for (const auto& block_proto : proto.output()) {
      mojom::ContentBlockPtr block;
      if (block_proto.has_text_content_block()) {
        auto text = mojom::TextContentBlock::New();
        if (auto value = ReadCompressibleString(
                block_proto.text_content_block().text())) {
          text->text = std::move(*value);
        }
        block = mojom::ContentBlock::NewTextContentBlock(std::move(text));
      } else if (block_proto.has_image_content_block()) {
        auto image = mojom::ImageContentBlock::New();
        image->image_url = GURL(block_proto.image_content_block().image_url());
        block = mojom::ContentBlock::NewImageContentBlock(std::move(image));
      } else if (block_proto.has_web_sources_content_block()) {
        block = mojom::ContentBlock::NewWebSourcesContentBlock(
            ProtoToWebSourcesContentBlock(
                block_proto.web_sources_content_block()));
      } else {
        continue;
      }
      blocks.push_back(std::move(block));
    }
    tool_use->output = std::move(blocks);
  }
  if (proto.artifacts_size() > 0) {
    std::vector<mojom::ToolArtifactPtr> artifacts;
    artifacts.reserve(proto.artifacts_size());
    for (const auto& a : proto.artifacts()) {
      auto artifact = mojom::ToolArtifact::New();
      artifact->type = a.type();
      if (auto value = ReadCompressibleString(a.content_json())) {
        artifact->content_json = std::move(*value);
      }
      artifacts.push_back(std::move(artifact));
    }
    tool_use->artifacts = std::move(artifacts);
  }
  if (proto.has_permission_challenge()) {
    auto pc = mojom::PermissionChallenge::New();
    if (proto.permission_challenge().has_assessment()) {
      pc->assessment = proto.permission_challenge().assessment();
    }
    if (proto.permission_challenge().has_plan()) {
      pc->plan = proto.permission_challenge().plan();
    }
    tool_use->permission_challenge = std::move(pc);
  }
  return tool_use;
}

mojom::ConversationEntryEventPtr ProtoToEntryEvent(
    const sync_pb::AIChatEntryEventProto& proto) {
  if (proto.has_completion()) {
    auto completion = mojom::CompletionEvent::New();
    if (auto value = ReadCompressibleString(proto.completion())) {
      completion->completion = std::move(*value);
    }
    return mojom::ConversationEntryEvent::NewCompletionEvent(
        std::move(completion));
  }
  if (proto.has_search_queries()) {
    auto event = mojom::SearchQueriesEvent::New();
    for (const auto& q : proto.search_queries().queries()) {
      event->search_queries.push_back(q);
    }
    return mojom::ConversationEntryEvent::NewSearchQueriesEvent(
        std::move(event));
  }
  if (proto.has_web_sources()) {
    auto event = mojom::WebSourcesEvent::New();
    for (const auto& source : proto.web_sources().sources()) {
      event->sources.push_back(ProtoToWebSource(source));
    }
    for (const auto& rr : proto.web_sources().rich_results()) {
      if (auto value = ReadCompressibleString(rr)) {
        event->rich_results.push_back(std::move(*value));
      }
    }
    return mojom::ConversationEntryEvent::NewSourcesEvent(std::move(event));
  }
  if (proto.has_inline_search()) {
    auto event = mojom::InlineSearchEvent::New();
    event->query = proto.inline_search().query();
    if (auto value =
            ReadCompressibleString(proto.inline_search().results_json())) {
      event->results_json = std::move(*value);
    }
    return mojom::ConversationEntryEvent::NewInlineSearchEvent(
        std::move(event));
  }
  if (proto.has_tool_use()) {
    return mojom::ConversationEntryEvent::NewToolUseEvent(
        ProtoToToolUse(proto.tool_use()));
  }
  return nullptr;
}

mojom::AssociatedContentPtr ProtoToAssociatedContent(
    const sync_pb::AIChatAssociatedContentProto& proto,
    const std::string& entry_uuid) {
  auto content = mojom::AssociatedContent::New();
  content->uuid = proto.uuid();
  content->title = proto.title();
  if (proto.has_url()) {
    content->url = GURL(proto.url());
  }
  content->content_type = static_cast<mojom::ContentType>(proto.content_type());
  content->content_used_percentage = proto.content_used_percentage();
  content->conversation_turn_uuid = entry_uuid;
  return content;
}

mojom::UploadedFilePtr ProtoToUploadedFile(
    const sync_pb::AIChatUploadedFile& proto) {
  auto file = mojom::UploadedFile::New();
  if (proto.has_filename()) {
    file->filename = proto.filename();
  }
  file->filesize = proto.filesize();
  file->type = static_cast<mojom::UploadedFileType>(proto.type());
  // Only populate bytes when the sender actually shipped them. When the
  // sender marked the field truncated, leave |data| empty so the caller can
  // preserve any existing local bytes.
  if (proto.has_data() && !proto.data_truncated_for_sync()) {
    const std::string& bytes = proto.data();
    file->data.assign(bytes.begin(), bytes.end());
  }
  if (proto.has_extracted_text()) {
    if (auto value = ReadCompressibleString(proto.extracted_text())) {
      file->extracted_text = std::move(*value);
    }
  }
  return file;
}

}  // namespace

sync_pb::AIChatConversationSpecifics ConversationMetadataToSpecifics(
    const mojom::Conversation& conversation) {
  sync_pb::AIChatConversationSpecifics specifics;
  auto* meta = specifics.mutable_conversation();
  meta->set_uuid(conversation.uuid);
  meta->set_title(conversation.title);
  if (conversation.model_key) {
    meta->set_model_key(*conversation.model_key);
  }
  meta->set_total_tokens(conversation.total_tokens);
  meta->set_trimmed_tokens(conversation.trimmed_tokens);
  meta->set_last_modified_time_unix_epoch_micros(
      conversation.updated_time.ToDeltaSinceWindowsEpoch().InMicroseconds());
  return specifics;
}

sync_pb::AIChatConversationSpecifics EntryToSpecifics(
    const std::string& conversation_uuid,
    const mojom::ConversationTurn& entry,
    const std::vector<mojom::AssociatedContentPtr>& associated_content,
    const base::flat_map<std::string, std::string>& associated_content_texts) {
  sync_pb::AIChatConversationSpecifics specifics;
  auto* entry_proto = specifics.mutable_entry();
  entry_proto->set_conversation_uuid(conversation_uuid);
  WriteEntryFields(entry, entry_proto);

  // Filter associated content to items tied to this entry only.
  if (entry.uuid) {
    for (const auto& content : associated_content) {
      if (!content->conversation_turn_uuid ||
          *content->conversation_turn_uuid != *entry.uuid) {
        continue;
      }
      std::optional<std::string_view> text;
      auto it = associated_content_texts.find(content->uuid);
      if (it != associated_content_texts.end()) {
        text = it->second;
      }
      WriteAssociatedContent(*content, text,
                             entry_proto->add_associated_content());
    }
  }

  return specifics;
}

mojom::ConversationPtr SpecificsToConversationMetadata(
    const sync_pb::AIChatConversationSpecifics& specifics) {
  if (!specifics.has_conversation()) {
    return nullptr;
  }
  const auto& meta = specifics.conversation();
  auto conversation = mojom::Conversation::New();
  conversation->uuid = meta.uuid();
  conversation->title = meta.title();
  if (meta.has_model_key()) {
    conversation->model_key = meta.model_key();
  }
  conversation->total_tokens = meta.total_tokens();
  conversation->trimmed_tokens = meta.trimmed_tokens();
  conversation->updated_time = base::Time::FromDeltaSinceWindowsEpoch(
      base::Microseconds(meta.last_modified_time_unix_epoch_micros()));
  conversation->has_content = true;
  return conversation;
}

mojom::ConversationTurnPtr SpecificsToEntry(
    const sync_pb::AIChatConversationSpecifics& specifics,
    std::vector<mojom::AssociatedContentPtr>* associated_content,
    base::flat_map<std::string, std::string>* associated_content_texts) {
  if (!specifics.has_entry()) {
    return nullptr;
  }
  const auto& proto = specifics.entry();
  auto entry = mojom::ConversationTurn::New();
  entry->uuid = proto.uuid();
  entry->created_time = base::Time::FromDeltaSinceWindowsEpoch(
      base::Microseconds(proto.date_unix_epoch_micros()));
  entry->text = proto.entry_text();
  if (proto.has_prompt()) {
    entry->prompt = proto.prompt();
  }
  entry->character_type =
      static_cast<mojom::CharacterType>(proto.character_type());
  entry->action_type = static_cast<mojom::ActionType>(proto.action_type());
  if (proto.has_selected_text()) {
    entry->selected_text = proto.selected_text();
  }
  if (proto.has_model_key()) {
    entry->model_key = proto.model_key();
  }

  if (proto.events_size() > 0) {
    std::vector<mojom::ConversationEntryEventPtr> events;
    events.reserve(proto.events_size());
    for (const auto& event_proto : proto.events()) {
      auto event = ProtoToEntryEvent(event_proto);
      if (event) {
        events.push_back(std::move(event));
      }
    }
    entry->events = std::move(events);
  }

  if (associated_content && proto.associated_content_size() > 0) {
    for (const auto& content_proto : proto.associated_content()) {
      associated_content->push_back(
          ProtoToAssociatedContent(content_proto, proto.uuid()));
      if (associated_content_texts && content_proto.has_last_contents()) {
        if (auto value =
                ReadCompressibleString(content_proto.last_contents())) {
          (*associated_content_texts)[content_proto.uuid()] = std::move(*value);
        }
        // When ReadCompressibleString returned nullopt (sentinel set), we
        // intentionally do not add an entry: the caller will then preserve
        // any existing local text for this AC.
      }
    }
  }

  if (proto.uploaded_files_size() > 0) {
    std::vector<mojom::UploadedFilePtr> files;
    files.reserve(proto.uploaded_files_size());
    for (const auto& file_proto : proto.uploaded_files()) {
      files.push_back(ProtoToUploadedFile(file_proto));
    }
    entry->uploaded_files = std::move(files);
  }
  return entry;
}

std::unique_ptr<syncer::EntityData> CreateEntityDataFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics) {
  auto entity_data = std::make_unique<syncer::EntityData>();
  *entity_data->specifics.mutable_ai_chat_conversation() = specifics;
  if (specifics.has_conversation()) {
    entity_data->name =
        base::StrCat({"conversation:", specifics.conversation().uuid()});
  } else if (specifics.has_entry()) {
    entity_data->name = base::StrCat({"entry:", specifics.entry().uuid()});
  }
  return entity_data;
}

std::string GetStorageKeyFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics) {
  if (specifics.has_conversation()) {
    return base::StrCat(
        {kConversationStorageKeyPrefix, specifics.conversation().uuid()});
  }
  if (specifics.has_entry()) {
    return base::StrCat({kEntryStorageKeyPrefix, specifics.entry().uuid()});
  }
  return std::string();
}

std::string GetClientTagFromSpecifics(
    const sync_pb::AIChatConversationSpecifics& specifics) {
  return GetStorageKeyFromSpecifics(specifics);
}

std::string GetStorageKeyFromEntitySpecifics(
    const sync_pb::EntitySpecifics& specifics) {
  return GetStorageKeyFromSpecifics(specifics.ai_chat_conversation());
}

}  // namespace ai_chat
