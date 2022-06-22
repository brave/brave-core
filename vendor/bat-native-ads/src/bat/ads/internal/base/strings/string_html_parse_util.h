/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_STRINGS_STRING_HTML_PARSE_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_STRINGS_STRING_HTML_PARSE_UTIL_H_

#include <string>

namespace ads {

std::string FindFirstRegexMatch(std::string& search_text, const std::string& rgx_str);

std::string ParseTagAttribute(const std::string& html, const std::string& tag_substr, const std::string& tag_attribute);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_STRINGS_STRING_STRING_HTML_PARSE_UTIL_H_
