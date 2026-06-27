/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/sync/ai_chat_sync_conversions.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/map_util.h"
#include "base/hash/hash.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/strcat.h"
#include "base/strings/string_view_util.h"
#include "base/system/sys_info.h"
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
  // Setting a real value supersedes any previous omitted_content_hash on |out|
  // (they share the same oneof), which matters on the merge path where an
  // AIChatCompressibleString the sender omitted is being restored.
  if (std::string compressed; value.size() >= kSyncCompressionThresholdBytes &&
                              compression::GzipCompress(value, &compressed) &&
                              compressed.size() < value.size()) {
    out->set_gzipped(std::move(compressed));
    return;
  }
  out->set_raw(value);
}

void OmitCompressibleString(sync_pb::AIChatCompressibleString* out) {
  // Replace the (possibly large) value with a hash of its content so the
  // receiver can restore it from a local copy that hashes identically. Reading
  // the current value handles both raw and gzipped storage; an already-omitted
  // or empty field hashes the empty string. Setting the hash arm of the oneof
  // clears any existing value.
  const std::string value =
      ReadCompressibleString(*out).value_or(std::string());
  out->set_omitted_content_hash(base::PersistentHash(value));
}

void OmitUploadedFileData(sync_pb::AIChatUploadedFile* file) {
  // Replace the raw bytes with a hash of them so the receiver can restore the
  // file from a local copy with identical content. Setting the hash arm of the
  // oneof clears |data|. Mirrors OmitCompressibleString for binary attachments.
  file->set_omitted_data_hash(base::PersistentHash(file->data()));
}

std::optional<std::string> ReadCompressibleString(
    const sync_pb::AIChatCompressibleString& in) {
  if (in.has_omitted_content_hash()) {
    // The sender omitted this field to fit the record budget; the caller
    // restores it from a local copy with identical content or preserves the
    // existing local value.
    return std::nullopt;
  }
  if (in.has_gzipped()) {
    // Limit maximum decompressed size to allowed maximum, or available memory.
    const uint32_t uncompressed_size =
        compression::GetUncompressedSize(in.gzipped());
    const size_t max_allowed_size = std::min(
        kSyncCompressionMaxDecompressedBytes,
        base::saturated_cast<size_t>(
            base::SysInfo::AmountOfAvailablePhysicalMemory().InBytes()));
    if (uncompressed_size > max_allowed_size) {
      DVLOG(1) << "Rejecting uncompressed string of size " << uncompressed_size
               << " bytes, exceeds max allowed " << max_allowed_size;
      return std::nullopt;
    }
    std::string out;
    if (!compression::GzipUncompress(in.gzipped(), &out)) {
      DVLOG(1) << "Rejecting gzipped string that failed to decompress";
      return std::nullopt;
    }
    return out;
  }
  if (in.has_raw()) {
    return in.raw();
  }
  DVLOG(1) << "Rejecting compressible string with no usable value";
  return std::nullopt;
}

bool FitEntryWithinSyncBudget(
    sync_pb::AIChatConversationSpecifics::Entry* entry) {
  if (entry->ByteSizeLong() <= kSyncMaxRecordBytes) {
    return true;
  }

  // Step 1: omit raw uploaded file bytes (least re-derivable, but already
  // unhelpful on the receiver without the local file present).
  for (auto& file : *entry->mutable_uploaded_files()) {
    if (file.has_data() && !file.data().empty()) {
      OmitUploadedFileData(&file);
    }
  }
  if (entry->ByteSizeLong() <= kSyncMaxRecordBytes) {
    return true;
  }

  // Step 2: associated_content.last_contents — the full page text. Most
  // re-derivable since the receiver still has the URL.
  for (auto& ac : *entry->mutable_associated_content()) {
    if (ac.has_last_contents() &&
        !ac.last_contents().has_omitted_content_hash()) {
      OmitCompressibleString(ac.mutable_last_contents());
    }
  }
  if (entry->ByteSizeLong() <= kSyncMaxRecordBytes) {
    return true;
  }

  // Step 3: uploaded_files.extracted_text — derivable from the file bytes
  // (which may already have been omitted, but that's fine).
  for (auto& file : *entry->mutable_uploaded_files()) {
    if (file.has_extracted_text() &&
        !file.extracted_text().has_omitted_content_hash()) {
      OmitCompressibleString(file.mutable_extracted_text());
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
          !src.page_content().has_omitted_content_hash()) {
        OmitCompressibleString(src.mutable_page_content());
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
      if (!text->has_omitted_content_hash()) {
        OmitCompressibleString(text);
      }
    }
  }
  if (entry->ByteSizeLong() <= kSyncMaxRecordBytes) {
    return true;
  }

  // Step 6: tool_use.arguments_json.
  for (auto& event : *entry->mutable_events()) {
    if (event.has_tool_use() && event.tool_use().has_arguments_json() &&
        !event.tool_use().arguments_json().has_omitted_content_hash()) {
      OmitCompressibleString(
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
      if (!rr.has_omitted_content_hash()) {
        OmitCompressibleString(&rr);
      }
    }
  }
  if (entry->ByteSizeLong() <= kSyncMaxRecordBytes) {
    return true;
  }

  // Step 8: completion text.
  for (auto& event : *entry->mutable_events()) {
    if (event.has_completion() &&
        !event.completion().has_omitted_content_hash()) {
      OmitCompressibleString(event.mutable_completion());
    }
  }
  if (entry->ByteSizeLong() <= kSyncMaxRecordBytes) {
    return true;
  }

  // Pathological: every omittable field has been omitted and the record is
  // still over budget. Refuse the commit.
  DLOG(ERROR) << "AI Chat entry " << entry->uuid()
              << " exceeds the sync record budget after omitting every "
              << "omittable field; refusing to commit. Size="
              << entry->ByteSizeLong();
  return false;
}

namespace {

// --- Upload helpers: mojom → sync proto ---

void WriteAssociatedContent(const mojom::AssociatedContent& content,
                            std::optional<std::string_view> last_contents,
                            sync_pb::AIChatAssociatedContentProto* proto) {
  // Not synced: content_id and tools_attached are runtime-only. The parent
  // linkage (conversation_turn_uuid) is implied by the entry this content is
  // nested under, so it is not written here.
  proto->set_uuid(content.uuid);
  proto->set_title(content.title);
  if (content.url.is_valid()) {
    proto->set_url(content.url.spec());
  }
  proto->set_content_type(std::to_underlying(content.content_type));
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
  // Not synced: permission_challenge is transient (resolved during the live
  // turn) and not persisted to the local store; each artifact's id is a local
  // identifier.
  proto->set_tool_name(tool_use.tool_name);
  proto->set_id(tool_use.id);
  WriteCompressibleString(tool_use.arguments_json,
                          proto->mutable_arguments_json());
  proto->set_is_server_result(tool_use.is_server_result);
  if (tool_use.output) {
    for (const auto& block : *tool_use.output) {
      auto* block_proto = proto->add_output();
      // No default case: adding a new ContentBlock variant must be an explicit
      // decision here rather than silently not syncing.
      switch (block->which()) {
        case mojom::ContentBlock::Tag::kTextContentBlock:
          WriteCompressibleString(
              block->get_text_content_block()->text,
              block_proto->mutable_text_content_block()->mutable_text());
          break;
        case mojom::ContentBlock::Tag::kImageContentBlock:
          block_proto->mutable_image_content_block()->set_image_url(
              block->get_image_content_block()->image_url.spec());
          break;
        case mojom::ContentBlock::Tag::kWebSourcesContentBlock:
          WriteWebSourcesContentBlock(
              *block->get_web_sources_content_block(),
              block_proto->mutable_web_sources_content_block());
          break;
        // Runtime-only content blocks that are intentionally not synced.
        case mojom::ContentBlock::Tag::kFileContentBlock:
        case mojom::ContentBlock::Tag::kFileExtractedTextContentBlock:
        case mojom::ContentBlock::Tag::kPageExcerptContentBlock:
        case mojom::ContentBlock::Tag::kPageTextContentBlock:
        case mojom::ContentBlock::Tag::kVideoTranscriptContentBlock:
        case mojom::ContentBlock::Tag::kRequestTitleContentBlock:
        case mojom::ContentBlock::Tag::kChangeToneContentBlock:
        case mojom::ContentBlock::Tag::kMemoryContentBlock:
        case mojom::ContentBlock::Tag::kFilterTabsContentBlock:
        case mojom::ContentBlock::Tag::kSuggestFocusTopicsContentBlock:
        case mojom::ContentBlock::Tag::kSuggestFocusTopicsWithEmojiContentBlock:
        case mojom::ContentBlock::Tag::kReduceFocusTopicsContentBlock:
        case mojom::ContentBlock::Tag::kSimpleRequestContentBlock:
          break;
      }
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
}

void WriteUploadedFile(const mojom::UploadedFile& file,
                       sync_pb::AIChatUploadedFile* proto) {
  proto->set_filename(file.filename);
  proto->set_filesize(file.filesize);
  proto->set_type(std::to_underlying(file.type));
  // Raw bytes are inlined here; the size-budget policy may later omit them
  // and set |omitted_data_hash| if the entry exceeds the size cap.
  if (!file.data.empty()) {
    proto->set_data(base::as_string_view(file.data));
  }
  if (file.extracted_text) {
    WriteCompressibleString(*file.extracted_text,
                            proto->mutable_extracted_text());
  }
}

void WriteEvent(const mojom::ConversationEntryEvent& event,
                sync_pb::AIChatEntryEventProto* proto) {
  // No default case: adding a new ConversationEntryEvent variant must be an
  // explicit decision here rather than silently not syncing.
  switch (event.which()) {
    case mojom::ConversationEntryEvent::Tag::kCompletionEvent:
      WriteCompressibleString(event.get_completion_event()->completion,
                              proto->mutable_completion());
      break;
    case mojom::ConversationEntryEvent::Tag::kSearchQueriesEvent:
      for (const auto& q : event.get_search_queries_event()->search_queries) {
        proto->mutable_search_queries()->add_queries(q);
      }
      break;
    case mojom::ConversationEntryEvent::Tag::kSourcesEvent: {
      auto* ws = proto->mutable_web_sources();
      const auto& sources_event = event.get_sources_event();
      for (const auto& source : sources_event->sources) {
        WriteWebSource(*source, ws->add_sources());
      }
      for (const auto& rr : sources_event->rich_results) {
        WriteCompressibleString(rr, ws->add_rich_results());
      }
      break;
    }
    case mojom::ConversationEntryEvent::Tag::kInlineSearchEvent: {
      auto* is = proto->mutable_inline_search();
      is->set_query(event.get_inline_search_event()->query);
      WriteCompressibleString(event.get_inline_search_event()->results_json,
                              is->mutable_results_json());
      break;
    }
    case mojom::ConversationEntryEvent::Tag::kToolUseEvent:
      WriteToolUse(*event.get_tool_use_event(), proto->mutable_tool_use());
      break;
    // Runtime-only / engine-response events that are intentionally not synced.
    case mojom::ConversationEntryEvent::Tag::kSearchStatusEvent:
    case mojom::ConversationEntryEvent::Tag::kDeepResearchEvent:
    case mojom::ConversationEntryEvent::Tag::kContentReceiptEvent:
    case mojom::ConversationEntryEvent::Tag::kConversationTitleEvent:
      break;
  }
}

void WriteEntryFields(const mojom::ConversationTurn& entry,
                      sync_pb::AIChatConversationSpecifics::Entry* proto) {
  // Not synced: from_brave_search_SERP is runtime-only (not persisted to the
  // local store). edits are not written here; EntryToSpecifics collapses them
  // into the latest revision before calling this.
  if (entry.uuid) {
    proto->set_uuid(*entry.uuid);
  }
  proto->set_date_unix_epoch_micros(
      entry.created_time.ToDeltaSinceWindowsEpoch().InMicroseconds());
  proto->set_entry_text(entry.text);
  if (entry.prompt) {
    proto->set_prompt(*entry.prompt);
  }
  proto->set_character_type(std::to_underlying(entry.character_type));
  proto->set_action_type(std::to_underlying(entry.action_type));
  if (entry.selected_text) {
    proto->set_selected_text(*entry.selected_text);
  }
  if (entry.model_key) {
    proto->set_model_key(*entry.model_key);
  }

  if (entry.events) {
    for (int order = 0; const auto& event : *entry.events) {
      auto* event_proto = proto->add_events();
      event_proto->set_event_order(order++);
      WriteEvent(*event, event_proto);
    }
  }

  if (entry.uploaded_files) {
    for (const auto& file : *entry.uploaded_files) {
      WriteUploadedFile(*file, proto->add_uploaded_files());
    }
  }

  if (entry.skill) {
    auto* skill_proto = proto->mutable_skill();
    skill_proto->set_shortcut(entry.skill->shortcut);
    skill_proto->set_prompt(entry.skill->prompt);
  }

  if (entry.near_verification_status) {
    proto->mutable_near_verification_status()->set_verified(
        entry.near_verification_status->verified);
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
  // Only populate bytes when the sender actually shipped them. When the sender
  // omitted them (the oneof holds omitted_data_hash instead), leave |data|
  // empty so the caller can restore any existing local bytes.
  if (proto.has_data()) {
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

// Populates |associated_content| from the entry's associated_content field
// (each tagged with the entry UUID) and, when provided, |associated_content_
// texts| with the last_contents value for each AC the sender included. An AC
// with omitted last_contents is intentionally left out of the texts map so the
// caller preserves any existing local text.
void ReadAssociatedContentFromEntry(
    const sync_pb::AIChatConversationSpecifics_Entry& proto,
    std::vector<mojom::AssociatedContentPtr>* associated_content,
    base::flat_map<std::string, std::string>* associated_content_texts) {
  if (!associated_content) {
    return;
  }
  for (const auto& content_proto : proto.associated_content()) {
    associated_content->push_back(
        ProtoToAssociatedContent(content_proto, proto.uuid()));
    if (associated_content_texts && content_proto.has_last_contents()) {
      if (auto value = ReadCompressibleString(content_proto.last_contents())) {
        (*associated_content_texts)[content_proto.uuid()] = std::move(*value);
      }
    }
  }
}

}  // namespace

sync_pb::AIChatConversationSpecifics ConversationMetadataToSpecifics(
    const mojom::Conversation& conversation) {
  // Not synced: updated_time and has_content are inferred locally from the
  // entries; temporary conversations are excluded from sync entirely;
  // associated_content is carried by the per-entry records, not here.
  sync_pb::AIChatConversationSpecifics specifics;
  auto* meta = specifics.mutable_conversation();
  meta->set_uuid(conversation.uuid);
  meta->set_title(conversation.title);
  if (conversation.model_key) {
    meta->set_model_key(*conversation.model_key);
  }
  meta->set_total_tokens(conversation.total_tokens);
  meta->set_trimmed_tokens(conversation.trimmed_tokens);
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

  // A turn that has been edited keeps its original text in |entry.text| and
  // stores each revision in |entry.edits|, with the most recent edit last.
  // That most recent edit is what the UI displays (see the untrusted
  // conversation UI, which renders `edits.at(-1)`), so it is the content we
  // sync. We serialize that content but keep the ORIGINAL turn's identity
  // (uuid + created_time) below, so the sync entity is stable across edits and
  // keeps its position in the conversation. Edit history itself is not synced
  // (only the latest revision); syncing the full edit/thread history is a
  // planned follow-up.
  const mojom::ConversationTurn& displayed =
      (entry.edits && !entry.edits->empty()) ? *entry.edits->back() : entry;
  WriteEntryFields(displayed, entry_proto);
  // Identity always comes from the original turn, not the edit.
  if (entry.uuid) {
    entry_proto->set_uuid(*entry.uuid);
  }
  entry_proto->set_date_unix_epoch_micros(
      entry.created_time.ToDeltaSinceWindowsEpoch().InMicroseconds());

  // Filter associated content to items tied to this entry only. Associated
  // content stays linked to the original turn's uuid even after an edit (the
  // associated content manager records only the first turn a content appears
  // with), so match on |entry.uuid|.
  if (entry.uuid) {
    for (const auto& content : associated_content) {
      if (!content->conversation_turn_uuid ||
          *content->conversation_turn_uuid != *entry.uuid) {
        continue;
      }
      std::optional<std::string_view> text;
      if (const std::string* found =
              base::FindOrNull(associated_content_texts, content->uuid)) {
        text = *found;
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

  ReadAssociatedContentFromEntry(proto, associated_content,
                                 associated_content_texts);

  if (proto.uploaded_files_size() > 0) {
    std::vector<mojom::UploadedFilePtr> files;
    files.reserve(proto.uploaded_files_size());
    for (const auto& file_proto : proto.uploaded_files()) {
      files.push_back(ProtoToUploadedFile(file_proto));
    }
    entry->uploaded_files = std::move(files);
  }

  if (proto.has_skill()) {
    entry->skill = mojom::SkillEntry::New(proto.skill().shortcut(),
                                          proto.skill().prompt());
  }
  if (proto.has_near_verification_status()) {
    entry->near_verification_status = mojom::NEARVerificationStatus::New(
        proto.near_verification_status().verified());
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
  // Specifics with neither kind set are rejected by IsEntityDataValid before
  // any storage-key/client-tag derivation runs.
  NOTREACHED();
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
