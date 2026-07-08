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
