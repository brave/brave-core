/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events.h"

#include <string>
#include <iostream>

#include "base/time/time.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_info.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_event_info.h"
#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_html_events_database_table.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/instance_id_util.h"
#include "bat/ads/internal/base/logging_util.h"

namespace ads {

void LogTextEmbeddingHTMLEvent(const TextEmbeddingInfo& text_embedding_info, TextEmbeddingHTMLEventCallback callback) {
  TextEmbeddingEventInfo text_embedding_html_event;
  text_embedding_html_event.timestamp = base::Time::Now();
  text_embedding_html_event.version = text_embedding_info.version;
  text_embedding_html_event.locale = text_embedding_info.locale;
  text_embedding_html_event.embedding = text_embedding_info.embedding;

  LogTextEmbeddingHTMLEvent(text_embedding_html_event, callback);
}

void LogTextEmbeddingHTMLEvent(const TextEmbeddingEventInfo& text_embedding_html_event, TextEmbeddingHTMLEventCallback callback) {
  // RecordAdEvent(text_embedding_html_event);

  database::table::TextEmbeddingHTMLEvents database_table;
  database_table.LogEvent(
      text_embedding_html_event, [callback](const bool success) { callback(success); });
}

// void PurgeExpiredAdEvents(TextEmbeddingHTMLEventCallback callback) {
//   database::table::AdEvents database_table;
//   database_table.PurgeExpired([callback](const bool success) {
//     if (success) {
//       RebuildAdEventsFromDatabase();
//     }

//     callback(success);
//   });
// }

// void PurgeOrphanedAdEvents(const mojom::AdType ad_type,
//                            TextEmbeddingHTMLEventCallback callback) {
//   database::table::AdEvents database_table;
//   database_table.PurgeOrphaned(ad_type, [callback](const bool success) {
//     if (success) {
//       RebuildAdEventsFromDatabase();
//     }

//     callback(success);
//   });
// }

// void RebuildAdEventsFromDatabase() {
//   database::table::AdEvents database_table;
//   database_table.GetAll([=](const bool success, const AdEventList& ad_events) {
//     if (!success) {
//       BLOG(1, "Failed to get ad events");
//       return;
//     }

//     const std::string& id = GetInstanceId();

//     AdsClientHelper::Get()->ResetAdEventsForId(id);

//     for (const auto& ad_event : ad_events) {
//       RecordAdEvent(ad_event);
//     }
//   });
// }

void GetTextEmbeddingEventsFromDatabase() {
  database::table::TextEmbeddingHTMLEvents database_table;
  database_table.GetAll([=](const bool success, const TextEmbeddingHTMLEventList& text_embedding_html_events) {
    if (!success) {
      BLOG(1, "Failed to get ad events");
      return;
    }

    for (const auto& text_embedding_html_event : text_embedding_html_events) {
      std::cout << "\n\n";
      std::cout << text_embedding_html_event.embedding;
    }
  });
}

// void RecordAdEvent(const TextEmbeddingEventInfo& text_embedding_html_event) {
//   AdsClientHelper::Get()->RecordAdEventForId(
//       GetInstanceId(), text_embedding_html_event.type.ToString(),
//       text_embedding_html_event.created_at);
// }

// std::vector<base::Time> GetAdEvents(const AdType& ad_type,
//                                     const ConfirmationType& confirmation_type) {
//   return AdsClientHelper::Get()->GetAdEvents(ad_type.ToString(),
//                                              confirmation_type.ToString());
// }

}  // namespace ads
