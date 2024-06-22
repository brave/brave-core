// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/common/brave_common_utils.h"

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"

namespace brave_utils {
std::u16string ReplaceChromeToBraveScheme(const std::u16string& url_string) {
  std::u16string new_url_string = url_string;
  ReplaceChromeToBraveScheme(&new_url_string);
  return new_url_string;
}

void ReplaceChromeToBraveScheme(std::u16string* url_string) {
  if (base::StartsWith(*url_string, kChromeSchema16,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    base::ReplaceFirstSubstringAfterOffset(url_string, 0, kChromeSchema16,
                                           kBraveSchema16);
  }

  return;
}

}  // namespace brave_utils
