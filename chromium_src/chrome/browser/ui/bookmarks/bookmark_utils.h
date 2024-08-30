
#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BOOKMARKS_BOOKMARKS_UTILS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BOOKMARKS_BOOKMARKS_UTILS_H_

#include "chrome/browser/ui/bookmarks/bookmark_utils.h"

// This replace is for changing the chrome:// to brave:// scheme displayed in
// Bookmarks Add page
#define FormatBookmarkURLForDisplay                          \
  FormatBookmarkURLForDisplay_ChromiumImpl(const GURL& url); \
  std::u16string FormatBookmarkURLForDisplay
#include "../../../../../../chrome/browser/ui/bookmarks/bookmark_utils.h"
#undef FormatBookmarkURLForDisplay
#endif