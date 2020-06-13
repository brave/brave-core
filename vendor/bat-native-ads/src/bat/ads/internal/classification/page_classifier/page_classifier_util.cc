/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/classification/page_classifier/page_classifier_util.h"  // NOLINT

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "third_party/re2/src/re2/re2.h"

namespace ads {
namespace classification {

std::string StripHtmlTagsAndNonAlphaCharacters(
    const std::string& content) {
  if (content.empty()) {
    return "";
  }

  std::string stripped_content = content;

  const std::string escaped_characters =
      RE2::QuoteMeta("!\"#$%&'()*+,-./:<=>?@\\[]^_`{|}~");

  const std::string pattern = base::StringPrintf("[[:cntrl:]]|"
      "\\\\(t|n|v|f|r)|[\\t\\n\\v\\f\\r]|\\\\x[[:xdigit:]][[:xdigit:]]|"
          "[%s]|\\S*\\d+\\S*", escaped_characters.c_str());

  RE2::GlobalReplace(&stripped_content, pattern, " ");

  return base::CollapseWhitespaceASCII(stripped_content, true);
}

}  // namespace classification
}  // namespace ads
