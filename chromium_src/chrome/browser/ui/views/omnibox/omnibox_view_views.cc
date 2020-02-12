/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/toolbar/brave_location_bar_model_delegate.h"
#include "brave/common/url_constants.h"
#include "content/public/common/url_constants.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "url/gurl.h"

namespace {

bool ShouldAdjust(const base::string16& original,
                  const base::string16& adjusted) {
  // Only adjust scheme is changed from brave to chrome.
  if (GURL(original).scheme() == kBraveUIScheme &&
      GURL(adjusted).scheme() == content::kChromeUIScheme) {
    return true;
  }

  return false;
}

void ReplaceChromeSchemeToBrave(GURL* url) {
  DCHECK_EQ(content::kChromeUIScheme, url->scheme());
  GURL::Replacements replacements;
  replacements.SetSchemeStr(kBraveUIScheme);
  *url = url->ReplaceComponents(replacements);
}

}  // namespace

#define HANDLE_CHROME_SCHEME_URL \
  base::string16 original_selected_text; \
  cb->ReadText(clipboard_buffer, &original_selected_text); \
  if (ShouldAdjust(original_selected_text, selected_text)) { \
    BraveLocationBarModelDelegate::FormattedStringFromURL(url, \
                                                          &selected_text); \
    ReplaceChromeSchemeToBrave(&url); \
  }

#include "../../../../../../../chrome/browser/ui/views/omnibox/omnibox_view_views.cc"  // NOLINT

#undef HANDLE_CHROME_SCHEME_URL
