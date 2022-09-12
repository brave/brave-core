/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events.h"

#include <vector>

#include "base/time/time.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_event_info.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events_database_table.h"

namespace ads {

void LogTextEmbeddingHtmlEvent(const std::string& embedding_as_string,
                               const std::string& hashed_text_base64,
                               TextEmbeddingHtmlEventCallback callback) {
  TextEmbeddingHtmlEventInfo text_embedding_html_event;
  text_embedding_html_event.created_at = base::Time::Now();
  text_embedding_html_event.version = {};
  text_embedding_html_event.locale = {};
  text_embedding_html_event.hashed_text_base64 = hashed_text_base64;
  text_embedding_html_event.embedding = embedding_as_string;

  database::table::TextEmbeddingHtmlEvents database_table;
  database_table.LogEvent(
      text_embedding_html_event,
      base::BindOnce([](TextEmbeddingHtmlEventCallback callback,
                        const bool success) { callback(success); },
                     callback));
}

void PurgeStaleTextEmbeddingHtmlEvents(
    TextEmbeddingHtmlEventCallback callback) {
  database::table::TextEmbeddingHtmlEvents database_table;
  database_table.PurgeStale(
      base::BindOnce([](TextEmbeddingHtmlEventCallback callback,
                        const bool success) { callback(success); },
                     callback));
}

void GetTextEmbeddingHtmlEventsFromDatabase(
    database::table::GetTextEmbeddingHtmlEventsCallback callback) {
  database::table::TextEmbeddingHtmlEvents database_table;
  database_table.GetAll(
      [=](const bool success,
          const TextEmbeddingHtmlEventList& text_embedding_html_events) {
        if (!success) {
          BLOG(1, "Failed to get text embedding HTML events");
          callback(success, {});
          return;
        }
        callback(success, text_embedding_html_events);
      });
}

}  // namespace ads
