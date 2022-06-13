/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/strings/string_html_parse_util.h"

namespace ads {

std::string ParseTagAttribute(const std::string& html, const std::string tag_substr, const std::string tag_attribute) {
  std::string attribute_text("");
  std::string attribute_str(tag_attribute + "=");
  std::size_t og_found = html.find(tag_substr);
  if (og_found != std::string::npos) {
    std::size_t tag_start = 0;
    for (int i = og_found; i >= 0; i--) {
      if (html[i] == '<') {
        tag_start = i;
        break;
      }
    }
    if (tag_start > 0) {
      int sub_str_len = attribute_str.length();
      std::size_t attribute_start = 0;
      for (std::size_t i = tag_start; i < html.length(); i++) {
        if (html.substr(i, sub_str_len) == attribute_str) {
          attribute_start = i + sub_str_len;
          break;
        }
      }
      if (attribute_start > 0) {
        for (std::size_t i = attribute_start; i < html.length(); i++) {
          if (html[i] == '>') {
            break;
          }
          attribute_text += html[i];
        }
      }
    }
  }
  return attribute_text;
}

}  // namespace ads
