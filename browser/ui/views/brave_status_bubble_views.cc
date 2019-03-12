/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_status_bubble_views.h"

#include "content/public/common/url_constants.h"
#include "url/gurl.h"

void BraveStatusBubbleViews::SetURL(const GURL& url) {
  GURL revised_url = url;
  if (revised_url.SchemeIs(content::kChromeUIScheme)) {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(content::kBraveUIScheme);
    revised_url = revised_url.ReplaceComponents(replacements);
  }

  StatusBubbleViews::SetURL(revised_url);
}
