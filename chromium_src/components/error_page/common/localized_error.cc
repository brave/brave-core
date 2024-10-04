/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/grit/brave_components_strings.h"
#include "components/url_formatter/url_formatter.h"
#include "url/gurl.h"

namespace error_page {
std::u16string GetFailedUrlString(GURL failed_url);
}  // namespace error_page

#define failed_url_string(FORMATTED_URL) \
  failed_url_string = error_page::GetFailedUrlString(failed_url);

#include "src/components/error_page/common/localized_error.cc"

#undef failed_url_string

namespace error_page {
namespace {
constexpr char kBraveUIScheme[] = "brave";
}

std::u16string GetFailedUrlString(GURL failed_url) {
  if (failed_url.scheme() == kChromeUIScheme) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(kBraveUIScheme);

    failed_url = failed_url.ReplaceComponents(replacements);
  }

  return url_formatter::FormatUrl(
      failed_url, url_formatter::kFormatUrlOmitNothing,
      base::UnescapeRule::NORMAL, nullptr, nullptr, nullptr);
}
}  // namespace error_page
