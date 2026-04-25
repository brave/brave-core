// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/brave_scheme_utils.h"

#include <string_view>

#include "base/strings/string_util.h"
#include "url/third_party/mozilla/url_parse.h"

namespace {
constexpr char16_t kChromeSchema16[] = u"chrome";
constexpr char16_t kBraveSchema16[] = u"brave";
}  // namespace

namespace brave_utils {

bool ReplaceChromeToBraveScheme(std::u16string& url_string) {
  url::Component scheme;
  if (url::ExtractScheme(url_string, &scheme) && scheme.is_valid()) {
    std::u16string_view scheme_view = scheme.AsViewOn(url_string);
    if (base::EqualsCaseInsensitiveASCII(scheme_view, kChromeSchema16)) {
      url_string.replace(scheme.begin, scheme.len, kBraveSchema16);
      return true;
    }
  }
  return false;
}

}  // namespace brave_utils
