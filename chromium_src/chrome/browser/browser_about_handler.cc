/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define FixupBrowserAboutURL FixupBrowserAboutURL_ChromiumImpl
#include "../../../../chrome/browser/browser_about_handler.cc"  // NOLINT
#undef FixupBrowserAboutURL

#include "brave/common/url_constants.h"
#include "brave/common/webui_url_constants.h"

bool FixupBrowserAboutURL(GURL* url,
                          content::BrowserContext* browser_context) {
  bool result = FixupBrowserAboutURL_ChromiumImpl(url, browser_context);

  if (url->SchemeIs(kBraveUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIScheme);
    *url = url->ReplaceComponents(replacements);
  }

  return result;
}
