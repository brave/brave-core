/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/libxml_utils.h"

#include <libxml/tree.h>

#include <string>
#include <string_view>

#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversion_utils.h"
#include "third_party/blink/renderer/platform/wtf/text/string_utf8_adaptor.h"

namespace brave_page_graph {

XmlUtf8String::XmlUtf8String(std::string_view str) {
  // XML doesn't allow invalid UTF-8 characters. Process it manually and replace
  // all invalid characters with a replacement code point.
  std::string xml_supported_utf8;
  if (!base::IsStringUTF8(str)) {
    constexpr base_icu::UChar32 kUnicodeReplacementPoint = 0xFFFD;
    xml_supported_utf8.reserve(str.size());
    for (size_t char_index = 0; char_index < str.size(); ++char_index) {
      base_icu::UChar32 code_point = CBU_SENTINEL;
      if (!base::ReadUnicodeCharacter(str.data(), str.size(), &char_index,
                                      &code_point) ||
          code_point == CBU_SENTINEL) {
        break;
      }
      if (!base::IsValidCharacter(code_point)) {
        code_point = kUnicodeReplacementPoint;
      }
      base::WriteUnicodeCharacter(code_point, &xml_supported_utf8);
    }
    str = xml_supported_utf8;
  }

  DCHECK(base::IsStringUTF8(str));
  xml_string_ =
      xmlCharStrndup(str.data(), base::saturated_cast<int>(str.size()));
}

XmlUtf8String::XmlUtf8String(const String& str)
    : XmlUtf8String(
          StringUTF8Adaptor(str, Utf8ConversionMode::kStrictReplacingErrors)
              .AsStringView()) {}

XmlUtf8String::XmlUtf8String(int value)
    : XmlUtf8String(base::NumberToString(value)) {}

XmlUtf8String::~XmlUtf8String() {
  xmlFree(xml_string_);
}

}  // namespace brave_page_graph
