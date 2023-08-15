/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_embedding/text_embedding_html_event_info.h"

namespace brave_ads {

TextEmbeddingHtmlEventInfo::TextEmbeddingHtmlEventInfo() = default;

TextEmbeddingHtmlEventInfo::TextEmbeddingHtmlEventInfo(
    const TextEmbeddingHtmlEventInfo& other) = default;

TextEmbeddingHtmlEventInfo& TextEmbeddingHtmlEventInfo::operator=(
    const TextEmbeddingHtmlEventInfo& other) = default;

TextEmbeddingHtmlEventInfo::TextEmbeddingHtmlEventInfo(
    TextEmbeddingHtmlEventInfo&& other) noexcept = default;

TextEmbeddingHtmlEventInfo& TextEmbeddingHtmlEventInfo::operator=(
    TextEmbeddingHtmlEventInfo&& other) noexcept = default;

TextEmbeddingHtmlEventInfo::~TextEmbeddingHtmlEventInfo() = default;

}  // namespace brave_ads
