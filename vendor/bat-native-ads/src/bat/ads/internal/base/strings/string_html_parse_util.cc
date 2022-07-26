/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/strings/string_html_parse_util.h"
#include "third_party/re2/src/re2/re2.h"

namespace ads {

std::string ParseTagAttribute(const std::string& html,
                              const std::string& tag_substr,
                              const std::string& tag_attribute) {
  std::string tag_text;
  std::string trailing;
  std::string attribute_text;

  re2::RE2::PartialMatch(
    html, "(<[^>]*" + tag_substr + "[^<]*>)", &tag_text);
  re2::RE2::PartialMatch(
    tag_text, "(" + tag_attribute + "=.*>)", &trailing);

  if (trailing.length() > tag_attribute.length() + 2) {
    const std::string delim = trailing.substr(tag_attribute.length() + 1, 1);
    re2::RE2::PartialMatch(
      trailing, "(" + delim + "[^" + delim + "]*" + delim + ")", &attribute_text);
    attribute_text = attribute_text.substr(1, attribute_text.length() - 2);
  }
  return attribute_text;
}

}  // namespace ads
