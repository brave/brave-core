/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_STRINGS_STRING_HTML_PARSER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_STRINGS_STRING_HTML_PARSER_UTIL_H_

#include <string>

namespace brave_ads {

std::string ParseHtmlTagNameAttribute(const std::string& html,
                                      const std::string& tag,
                                      const std::string& name_attribute);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_STRINGS_STRING_HTML_PARSER_UTIL_H_
