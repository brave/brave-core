/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENT_INFO_H_

#include <string>
#include <vector>

#include "base/time/time.h"

namespace ads {

struct TextEmbeddingHtmlEventInfo final {
  TextEmbeddingHtmlEventInfo();

  TextEmbeddingHtmlEventInfo(const TextEmbeddingHtmlEventInfo& other);
  TextEmbeddingHtmlEventInfo& operator=(
      const TextEmbeddingHtmlEventInfo& other);

  TextEmbeddingHtmlEventInfo(TextEmbeddingHtmlEventInfo&& other) noexcept;
  TextEmbeddingHtmlEventInfo& operator=(
      TextEmbeddingHtmlEventInfo&& other) noexcept;

  ~TextEmbeddingHtmlEventInfo();

  base::Time created_at;
  std::string locale;
  std::string hashed_text_base64;
  std::string embedding;
};

using TextEmbeddingHtmlEventList = std::vector<TextEmbeddingHtmlEventInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENT_INFO_H_
