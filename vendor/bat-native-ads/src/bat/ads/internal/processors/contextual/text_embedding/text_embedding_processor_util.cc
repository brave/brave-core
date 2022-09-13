/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_processor_util.h"

#include <algorithm>

#include "base/strings/string_util.h"
#include "bat/ads/internal/base/strings/string_html_parser_util.h"
#include "bat/ads/internal/base/strings/string_strip_util.h"

namespace ads::processor {

std::string SanitizeHtml(const std::string& html) {
  return SanitizeText(ParseHtmlTagAttribute(html, "og:title", "content"));
}

std::string SanitizeText(const std::string& text) {
  return base::ToLowerASCII(StripNonAlphaCharacters(text));
}

}  // namespace ads::processor
