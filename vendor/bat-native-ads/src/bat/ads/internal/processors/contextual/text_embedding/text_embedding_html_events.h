/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENTS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENTS_H_

#include <functional>
#include <string>

#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

struct TextEmbeddingEventInfo;

using TextEmbeddingHtmlEventCallback = std::function<void(const bool)>;

void LogTextEmbeddingHtmlEvent(const std::string embedding_formatted,
                               const std::string hashed_key,
                               TextEmbeddingHtmlEventCallback callback);
void PurgeStaleTextEmbeddingHtmlEvents(TextEmbeddingHtmlEventCallback callback);
void GetTextEmbeddingEventsFromDatabase();

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENTS_H_
