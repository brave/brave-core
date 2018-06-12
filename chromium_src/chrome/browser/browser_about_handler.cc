/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define FixupBrowserAboutURL FixupBrowserAboutURL_ChromiumImpl
#define WillHandleBrowserAboutURL WillHandleBrowserAboutURL_ChromiumImpl
#include "../../../../chrome/browser/browser_about_handler.cc"
#undef FixupBrowserAboutURL
#undef WillHandleBrowserAboutURL

#include "brave/common/url_constants.h"

bool FixupBrowserAboutURLBraveImpl(GURL* url,
                                   content::BrowserContext* browser_context) {
  if (url->SchemeIs(kBraveUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kChromeUIScheme);
    *url = url->ReplaceComponents(replacements);
  }
  return true;
}


bool FixupBrowserAboutURL(GURL* url,
                          content::BrowserContext* browser_context) {
  FixupBrowserAboutURLBraveImpl(url, browser_context);
  return FixupBrowserAboutURL_ChromiumImpl(url, browser_context);
}

bool WillHandleBrowserAboutURL(GURL* url,
                               content::BrowserContext* browser_context) {
  FixupBrowserAboutURLBraveImpl(url, browser_context);
  return WillHandleBrowserAboutURL_ChromiumImpl(url, browser_context);
}
