/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENTS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENTS_H_

#include <functional>
#include <vector>

#include "bat/ads/public/interfaces/ads.mojom.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

struct TextEmbeddingInfo;
struct TextEmbeddingEventInfo;

using TextEmbeddingHTMLEventCallback = std::function<void(const bool)>;

void LogTextEmbeddingHTMLEvent(const TextEmbeddingInfo& text_embedding_info, TextEmbeddingHTMLEventCallback callback);

void LogTextEmbeddingHTMLEvent(const TextEmbeddingEventInfo& text_embedding_html_event, TextEmbeddingHTMLEventCallback callback);

// void PurgeExpiredAdEvents(TextEmbeddingHTMLEventCallback callback);
// void PurgeOrphanedAdEvents(const mojom::AdType ad_type,
//                            TextEmbeddingHTMLEventCallback callback);

// void RebuildAdEventsFromDatabase();

void GetTextEmbeddingEventsFromDatabase();

// void RecordAdEvent(const TextEmbeddingEventInfo& text_embedding_html_event);

// std::vector<base::Time> GetAdEvents(const AdType& ad_type,
//                                     const ConfirmationType& confirmation_type);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENTS_H_
