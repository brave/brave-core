/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/strings/string_html_parse_util.h"
#include "base/strings/strcat.h"
#include "third_party/re2/src/re2/re2.h"

namespace ads {

std::string ParseTagAttribute(const std::string& html,
                              const std::string& tag_substr,
                              const std::string& tag_attribute) {
  std::string tag_text;
  re2::RE2::PartialMatch(html, base::StrCat({"(<[^>]*", tag_substr, "[^<]*>)"}),
                         &tag_text);

  std::string trailing;
  re2::RE2::PartialMatch(tag_text, base::StrCat({"(", tag_attribute, "=.*>)"}),
                         &trailing);

  std::string attribute_text;
  const int prefix_padding = 2;
  if (trailing.length() > tag_attribute.length() + prefix_padding) {
    const std::string delimiter =
        trailing.substr(tag_attribute.length() + 1, 1);
    re2::RE2::PartialMatch(
        trailing,
        base::StrCat({"(", delimiter, "[^", delimiter, "]*", delimiter, ")"}),
        &attribute_text);
    attribute_text =
        attribute_text.substr(1, attribute_text.length() - prefix_padding);
  }
  return attribute_text;
}

}  // namespace ads
