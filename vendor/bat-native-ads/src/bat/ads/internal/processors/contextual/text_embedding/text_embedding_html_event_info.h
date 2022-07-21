/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENT_INFO_H_

#include <string>

#include "base/time/time.h"
#include "url/gurl.h"

namespace ads {

struct TextEmbeddingEventInfo final {
  TextEmbeddingEventInfo();
  TextEmbeddingEventInfo(const TextEmbeddingEventInfo& info);
  TextEmbeddingEventInfo& operator=(const TextEmbeddingEventInfo& info);
  ~TextEmbeddingEventInfo();

  base::Time timestamp;
  std::string version;
  std::string locale;
  std::string hashed_key;
  std::string embedding;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_PROCESSORS_CONTEXTUAL_TEXT_EMBEDDING_TEXT_EMBEDDING_HTML_EVENT_INFO_H_
