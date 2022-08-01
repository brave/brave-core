/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_processor_util.h"

#include <algorithm>
#include <string>

#include "base/strings/string_util.h"
#include "bat/ads/internal/base/strings/string_html_parse_util.h"
#include "bat/ads/internal/base/strings/string_strip_util.h"

namespace ads {
namespace processor {

std::string SanitizeText(const std::string& text, bool is_html) {
  std::string sanitized_text = text;
  if (is_html) {
    sanitized_text = ParseTagAttribute(sanitized_text, "og:title", "content");
  }
  sanitized_text = StripNonAlphaCharacters(sanitized_text);
  std::transform(sanitized_text.begin(), sanitized_text.end(),
                 sanitized_text.begin(), ::tolower);
  return sanitized_text;
}

}  // namespace processor
}  // namespace ads
