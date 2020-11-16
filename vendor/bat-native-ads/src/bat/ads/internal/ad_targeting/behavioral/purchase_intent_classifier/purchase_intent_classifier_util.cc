/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/behavioral/purchase_intent_classifier/purchase_intent_classifier_util.h"

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/re2/src/re2/re2.h"

namespace ads {
namespace ad_targeting {
namespace behavioral {

std::string StripHtmlTagsAndNonAlphaNumericCharacters(
    const std::string& text) {
  if (text.empty()) {
    return "";
  }

  std::string stripped_text = text;

  const std::string escaped_characters =
      RE2::QuoteMeta("!\"#$%&'()*+,-./:<=>?@\\[]^_`{|}~");

  const std::string pattern = base::StringPrintf("[[:cntrl:]]|"
      "\\\\(t|n|v|f|r)|[\\t\\n\\v\\f\\r]|\\\\x[[:xdigit:]][[:xdigit:]]|"
          "[%s]", escaped_characters.c_str());

  RE2::GlobalReplace(&stripped_text, pattern, " ");

  base::string16 stripped_text_string16 =
      base::UTF8ToUTF16(stripped_text);

  stripped_text_string16 =
      base::CollapseWhitespace(stripped_text_string16, true);

  return base::UTF16ToUTF8(stripped_text_string16);
}

}  // namespace behavioral
}  // namespace ad_targeting
}  // namespace ads
