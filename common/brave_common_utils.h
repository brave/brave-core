// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMMON_BRAVE_COMMON_UTILS_H_
#define BRAVE_COMMON_BRAVE_COMMON_UTILS_H_

#include <string>

namespace brave_utils {
inline constexpr char16_t kChromeSchema16[] = u"chrome://";
inline constexpr char16_t kBraveSchema16[] = u"brave://";

std::u16string ReplaceChromeToBraveScheme(const std::u16string& url_string);
void ReplaceChromeToBraveScheme(std::u16string* url_string);
}  // namespace brave_utils

#endif  // BRAVE_COMMON_BRAVE_COMMON_UTILS_H_
