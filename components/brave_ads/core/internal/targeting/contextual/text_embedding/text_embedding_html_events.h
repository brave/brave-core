/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENTS_H_

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/ml/pipeline/text_processing/embedding_info.h"
#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_events_database_table.h"

namespace brave_ads {

using LogTextEmbeddingHtmlEventCallback =
    base::OnceCallback<void(bool success)>;

struct TextEmbeddingHtmlEventInfo;

TextEmbeddingHtmlEventInfo BuildTextEmbeddingHtmlEvent(
    const ml::pipeline::TextEmbeddingInfo& text_embedding);

void LogTextEmbeddingHtmlEvent(
    const TextEmbeddingHtmlEventInfo& text_embedding_html_event,
    LogTextEmbeddingHtmlEventCallback callback);

void PurgeStaleTextEmbeddingHtmlEvents(
    LogTextEmbeddingHtmlEventCallback callback);

void GetTextEmbeddingHtmlEventsFromDatabase(
    database::table::GetTextEmbeddingHtmlEventsCallback callback);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENTS_H_
