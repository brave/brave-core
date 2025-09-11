/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TABS_TAB_CONTENT_EXTRACTOR_H_
#define BRAVE_BROWSER_TABS_TAB_CONTENT_EXTRACTOR_H_

#include <string>

#include "base/functional/callback.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace tab_content_extractor {

// Extracts text content from a tab using DOM distiller.
// If the tab is not alive (discarded or dead render frame), it will be loaded
// first. The callback is called with the extracted content (or empty string on
// failure).
void ExtractTextContent(
    content::WebContents* web_contents,
    const GURL& tab_url,
    int tab_index,
    base::OnceCallback<void(std::pair<int, std::string>)> callback);

}  // namespace tab_content_extractor

#endif  // BRAVE_BROWSER_TABS_TAB_CONTENT_EXTRACTOR_H_
