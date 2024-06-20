// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/web_applications/app_browser_controller.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/url_constants.h"

#define FormatUrlOrigin FormatUrlOrigin_ChromiumImpl
#include "src/chrome/browser/ui/web_applications/app_browser_controller.cc"
#undef FormatUrlOrigin

namespace web_app {

// static
std::u16string AppBrowserController::FormatUrlOrigin(
    const GURL& url,
    url_formatter::FormatUrlTypes format_types) {
  std::u16string modified_url = FormatUrlOrigin_ChromiumImpl(url, format_types);

  const std::u16string kChromeUISchemeU16 =
      base::ASCIIToUTF16(base::StrCat({content::kChromeUIScheme, "://"}));

  if (base::StartsWith(modified_url, kChromeUISchemeU16,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    base::ReplaceFirstSubstringAfterOffset(
        &modified_url, 0ul, kChromeUISchemeU16,
        base::ASCIIToUTF16(base::StrCat({content::kBraveUIScheme, "://"})));
  }

  return modified_url;
}

}  // namespace web_app
