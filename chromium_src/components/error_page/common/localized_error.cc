/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/grit/brave_components_strings.h"
#include "components/url_formatter/url_formatter.h"
#include "url/gurl.h"

#define BRAVE_LOOKUP_ERROR_MAP                             \
  if (error_code == net::ERR_INCOGNITO_IPFS_NOT_ALLOWED) { \
    static LocalizedErrorMap error({                       \
        net::ERR_INCOGNITO_IPFS_NOT_ALLOWED,               \
        IDS_ERRORPAGES_IPFS_INCOGNITO_HEADING,             \
        IDS_ERRORPAGES_IPFS_INCOGNITO_SUMMARY,             \
        SUGGEST_NONE,                                      \
        SHOW_NO_BUTTONS,                                   \
    });                                                    \
    return &error;                                         \
  } else if (error_code == net::ERR_IPFS_DISABLED) {       \
    static LocalizedErrorMap error({                       \
        net::ERR_IPFS_DISABLED,                            \
        IDS_ERRORPAGES_IPFS_DISABLED_HEADING,              \
        IDS_ERRORPAGES_IPFS_DISABLED_SUMMARY,              \
        SUGGEST_NONE,                                      \
        SHOW_BUTTON_RELOAD,                                \
    });                                                    \
    return &error;                                         \
  }

namespace error_page {
std::u16string GetFailedUrlString(GURL failed_url);
}  // namespace error_page

#define failed_url_string(FORMATTED_URL) \
  failed_url_string = error_page::GetFailedUrlString(failed_url);

#include "src/components/error_page/common/localized_error.cc"

#undef failed_url_string
#undef BRAVE_LOOKUP_ERROR_MAP

namespace error_page {
namespace {
const char kBraveUIScheme[] = "brave";
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
