/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/strings/string_html_parser_util.h"

#include "base/strings/strcat.h"
#include "third_party/re2/src/re2/re2.h"

namespace {
constexpr int kPrefixPadding = 2;
}  // namespace

namespace brave_ads {

std::string ParseHtmlTagNameAttribute(const std::string& html,
                                      const std::string& tag,
                                      const std::string& name_attribute) {
  std::string tag_value;
  re2::RE2::PartialMatch(html, base::StrCat({"(<[^>]*", tag, "[^<]*>)"}),
                         &tag_value);

  std::string text;
  re2::RE2::PartialMatch(tag_value,
                         base::StrCat({"(", name_attribute, "=.*>)"}), &text);

  if (text.length() <= name_attribute.length() + kPrefixPadding) {
    return {};
  }

  const std::string delimiter = text.substr(name_attribute.length() + 1, 1);

  const std::string r =
      base::StrCat({"(", delimiter, "[^", delimiter, "]*", delimiter, ")"});

  std::string name_attribute_value;
  if (!re2::RE2::PartialMatch(text, r, &name_attribute_value)) {
    return {};
  }

  return name_attribute_value.substr(
      1, name_attribute_value.length() - kPrefixPadding);
}

}  // namespace brave_ads
