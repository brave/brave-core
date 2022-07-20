/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/strings/string_html_parse_util.h"

#include <regex>

namespace ads {

std::string FindFirstRegexMatch(std::string& search_text, const std::string& rgx_str) {
  std::string match_str;
  std::regex rgx(rgx_str);
  std::smatch match;
  while (std::regex_search(search_text, match, rgx)) {
    for (auto x:match) {
      match_str = x;
      break;
    }
    if (match_str.length() > 0) {
      break;
    }
    match_str = match.suffix().str();
  }
  return match_str;
}

std::string ParseTagAttribute(const std::string& html, const std::string& tag_substr, const std::string& tag_attribute) {
  std::string attribute_text;
  std::string search_text = html;
  attribute_text = FindFirstRegexMatch(search_text, "<[^>]*" + tag_substr + "[^<]*>");
  attribute_text = FindFirstRegexMatch(attribute_text, tag_attribute + "=.*>");
  if (attribute_text.length() > tag_attribute.length()) {
    attribute_text = attribute_text.substr(tag_attribute.length(), attribute_text.length() - tag_attribute.length());
  }
  return attribute_text;
}

}  // namespace ads
