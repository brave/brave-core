/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_events.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_info.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_events_database_table.h"

namespace brave_ads {

namespace {

void GetTextEmbeddingHtmlEventsFromDatabaseCallback(
    database::table::GetTextEmbeddingHtmlEventsCallback callback,
    const bool success,
    const TextEmbeddingHtmlEventList& text_embedding_html_events) {
  if (!success) {
    BLOG(1, "Failed to get text embedding HTML events");
  }

  std::move(callback).Run(success, text_embedding_html_events);
}

}  // namespace

TextEmbeddingHtmlEventInfo BuildTextEmbeddingHtmlEvent(
    const ml::pipeline::TextEmbeddingInfo& text_embedding) {
  TextEmbeddingHtmlEventInfo text_embedding_html_event;

  text_embedding_html_event.created_at = base::Time::Now();
  text_embedding_html_event.locale = text_embedding.locale;
  text_embedding_html_event.hashed_text_base64 =
      text_embedding.hashed_text_base64;
  text_embedding_html_event.embedding = text_embedding.embedding;

  return text_embedding_html_event;
}

void LogTextEmbeddingHtmlEvent(
    const TextEmbeddingHtmlEventInfo& text_embedding_html_event,
    LogTextEmbeddingHtmlEventCallback callback) {
  database::table::TextEmbeddingHtmlEvents database_table;
  database_table.LogEvent(
      text_embedding_html_event,
      base::BindOnce(
          [](LogTextEmbeddingHtmlEventCallback callback, const bool success) {
            std::move(callback).Run(success);
          },
          std::move(callback)));
}

void PurgeStaleTextEmbeddingHtmlEvents(
    LogTextEmbeddingHtmlEventCallback callback) {
  const database::table::TextEmbeddingHtmlEvents database_table;
  database_table.PurgeStale(base::BindOnce(
      [](LogTextEmbeddingHtmlEventCallback callback, const bool success) {
        std::move(callback).Run(success);
      },
      std::move(callback)));
}

void GetTextEmbeddingHtmlEventsFromDatabase(
    database::table::GetTextEmbeddingHtmlEventsCallback callback) {
  const database::table::TextEmbeddingHtmlEvents database_table;
  database_table.GetAll(base::BindOnce(
      &GetTextEmbeddingHtmlEventsFromDatabaseCallback, std::move(callback)));
}

}  // namespace brave_ads
