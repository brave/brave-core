/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events.h"

#include <string>
#include <vector>

#include "base/time/time.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/instance_id_util.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_event_info.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events_database_table.h"

namespace ads {

void LogTextEmbeddingHtmlEvent(const std::string embedding_formatted,
                               const std::string hashed_key,
                               TextEmbeddingHtmlEventCallback callback) {
  TextEmbeddingEventInfo text_embedding_event_info;
  text_embedding_event_info.created_at = base::Time::Now();
  text_embedding_event_info.version = "";
  text_embedding_event_info.locale = "";
  text_embedding_event_info.hashed_key = hashed_key;
  text_embedding_event_info.embedding = embedding_formatted;

  database::table::TextEmbeddingHtmlEvents database_table;
  database_table.LogEvent(
      text_embedding_event_info,
      [callback](const bool success) { callback(success); });
}

void PurgeStaleTextEmbeddingHtmlEvents(
    TextEmbeddingHtmlEventCallback callback) {
  database::table::TextEmbeddingHtmlEvents database_table;
  database_table.PurgeStale(
      [callback](const bool success) { callback(success); });
}

void GetTextEmbeddingEventsFromDatabase(
    database::table::GetTextEmbeddingHtmlEventsCallback callback) {
  database::table::TextEmbeddingHtmlEvents database_table;
  database_table.GetAll(
      [=](const bool success,
          const TextEmbeddingHtmlEventList& text_embedding_html_events) {
        if (!success) {
          BLOG(1, "Failed to get embeddings");
          callback(success, {});
          return;
        }
        callback(success, text_embedding_html_events);
      });
}

}  // namespace ads
