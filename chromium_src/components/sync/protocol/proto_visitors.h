/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PROTOCOL_PROTO_VISITORS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PROTOCOL_PROTO_VISITORS_H_

#include "brave/components/sync/protocol/ai_chat_specifics.pb.h"

#define BRAVE_VISIT_DEVICE_INFO_SPECIFICS_BRAVE_FIELDS \
VISIT(brave_fields);

#define BRAVE_VISIT_PROTO_FIELDS_BRAVE_SPECIFIC_FIELD                      \
  VISIT_PROTO_FIELDS(const sync_pb::BraveSpecificFields& proto) {          \
    VISIT(is_self_delete_supported);                                       \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatCompressibleString& proto) {     \
    VISIT(raw);                                                            \
    VISIT_BYTES(gzipped);                                                  \
    VISIT(omitted_content_hash);                                           \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatConversationSpecifics& proto) {  \
    VISIT(conversation);                                                   \
    VISIT(entry);                                                          \
  }                                                                        \
  VISIT_PROTO_FIELDS(                                                      \
      const sync_pb::AIChatConversationSpecifics_Conversation& proto) {    \
    VISIT(uuid);                                                           \
    VISIT(title);                                                          \
    VISIT(model_key);                                                      \
    VISIT(total_tokens);                                                   \
    VISIT(trimmed_tokens);                                                 \
  }                                                                        \
  VISIT_PROTO_FIELDS(                                                      \
      const sync_pb::AIChatConversationSpecifics_Entry& proto) {           \
    VISIT(uuid);                                                           \
    VISIT(conversation_uuid);                                              \
    VISIT(date_unix_epoch_micros);                                         \
    VISIT(entry_text);                                                     \
    VISIT(prompt);                                                         \
    VISIT(character_type);                                                 \
    VISIT(editing_entry_uuid);                                             \
    VISIT(action_type);                                                    \
    VISIT(selected_text);                                                  \
    VISIT(model_key);                                                      \
    VISIT_REP(events);                                                     \
    VISIT_REP(associated_content);                                         \
    VISIT_REP(uploaded_files);                                             \
    VISIT(skill);                                                          \
    VISIT(near_verification_status);                                       \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatSkillEntry& proto) {             \
    VISIT(shortcut);                                                       \
    VISIT(prompt);                                                         \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatNEARVerificationStatus& proto) { \
    VISIT(verified);                                                       \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatAssociatedContentProto& proto) { \
    VISIT(uuid);                                                           \
    VISIT(title);                                                          \
    VISIT(url);                                                            \
    VISIT(content_type);                                                   \
    VISIT(content_used_percentage);                                        \
    VISIT(last_contents);                                                  \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatUploadedFile& proto) {           \
    VISIT(filename);                                                       \
    VISIT(filesize);                                                       \
    VISIT(type);                                                           \
    VISIT_BYTES(data);                                                     \
    VISIT(omitted_data_hash);                                              \
    VISIT(extracted_text);                                                 \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatEntryEventProto& proto) {        \
    VISIT(event_order);                                                    \
    VISIT(completion);                                                     \
    VISIT(search_queries);                                                 \
    VISIT(web_sources);                                                    \
    VISIT(inline_search);                                                  \
    VISIT(tool_use);                                                       \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatSearchQueriesEvent& proto) {     \
    VISIT_REP(queries);                                                    \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatWebSourcesEvent& proto) {        \
    VISIT_REP(sources);                                                    \
    VISIT_REP(rich_results);                                               \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatWebSource& proto) {              \
    VISIT(title);                                                          \
    VISIT(url);                                                            \
    VISIT(favicon_url);                                                    \
    VISIT(page_content);                                                   \
    VISIT_REP(extra_snippets);                                             \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatInlineSearchEvent& proto) {      \
    VISIT(query);                                                          \
    VISIT(results_json);                                                   \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatToolUseEvent& proto) {           \
    VISIT(tool_name);                                                      \
    VISIT(id);                                                             \
    VISIT(arguments_json);                                                 \
    VISIT_REP(output);                                                     \
    VISIT(is_server_result);                                               \
    VISIT_REP(artifacts);                                                  \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatContentBlock& proto) {           \
    VISIT(image_content_block);                                            \
    VISIT(text_content_block);                                             \
    VISIT(web_sources_content_block);                                      \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatImageContentBlock& proto) {      \
    VISIT(image_url);                                                      \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatTextContentBlock& proto) {       \
    VISIT(text);                                                           \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatWebSourcesContentBlock& proto) { \
    VISIT_REP(sources);                                                    \
    VISIT_REP(queries);                                                    \
    VISIT_REP(rich_results);                                               \
  }                                                                        \
  VISIT_PROTO_FIELDS(const sync_pb::AIChatToolArtifact& proto) {           \
    VISIT(type);                                                           \
    VISIT(content_json);                                                   \
  }

#include <components/sync/protocol/proto_visitors.h>  // IWYU pragma: export
#undef BRAVE_VISIT_PROTO_FIELDS_BRAVE_SPECIFIC_FIELD
#undef BRAVE_VISIT_DEVICE_INFO_SPECIFICS_BRAVE_FIELDS

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_PROTOCOL_PROTO_VISITORS_H_
