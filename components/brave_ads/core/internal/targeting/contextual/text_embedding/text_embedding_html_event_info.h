/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENT_INFO_H_

#include <string>
#include <vector>

#include "base/time/time.h"

namespace brave_ads {

struct TextEmbeddingHtmlEventInfo final {
  TextEmbeddingHtmlEventInfo();

  TextEmbeddingHtmlEventInfo(const TextEmbeddingHtmlEventInfo&);
  TextEmbeddingHtmlEventInfo& operator=(const TextEmbeddingHtmlEventInfo&);

  TextEmbeddingHtmlEventInfo(TextEmbeddingHtmlEventInfo&&) noexcept;
  TextEmbeddingHtmlEventInfo& operator=(TextEmbeddingHtmlEventInfo&&) noexcept;

  ~TextEmbeddingHtmlEventInfo();

  bool operator==(const TextEmbeddingHtmlEventInfo&) const = default;

  base::Time created_at;
  std::string locale;
  std::string hashed_text_base64;
  std::vector<float> embedding;
};

using TextEmbeddingHtmlEventList = std::vector<TextEmbeddingHtmlEventInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENT_INFO_H_
