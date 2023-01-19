/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/processors/contextual/text_embedding/text_embedding_processor_util.h"

#include "base/strings/string_util.h"
#include "bat/ads/internal/common/strings/string_html_parser_util.h"
#include "bat/ads/internal/common/strings/string_strip_util.h"

namespace ads::processor {

namespace {

constexpr char kTagName[] = "og:title";
constexpr char kAttributeName[] = "content";

}  // namespace

std::string SanitizeHtml(const std::string& html) {
  return SanitizeText(ParseHtmlTagAttribute(html, kTagName, kAttributeName));
}

std::string SanitizeText(const std::string& text) {
  return base::ToLowerASCII(StripNonAlphaCharacters(text));
}

}  // namespace ads::processor
