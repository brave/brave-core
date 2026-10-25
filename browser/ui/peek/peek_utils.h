/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_PEEK_PEEK_UTILS_H_
#define BRAVE_BROWSER_UI_PEEK_PEEK_UTILS_H_

class GURL;

namespace content {
struct OpenURLParams;
}  // namespace content

namespace peek {

// Only http(s) pages can be shown in Peek.
bool IsPeekableURL(const GURL& url);

// True when `from` and `to` belong to different sites (eTLD+1).
bool IsCrossSite(const GURL& from, const GURL& to);

// Whether a user-initiated link from a tab showing `source_url` to
// `target_url` should open in Peek because the tab is pinned.
bool ShouldPeekFromPinnedTab(bool is_pinned,
                             const GURL& source_url,
                             const GURL& target_url);

// Whether `params` come from a Shift+click on a link in a page.
bool IsShiftClickOpen(const content::OpenURLParams& params);

}  // namespace peek

#endif  // BRAVE_BROWSER_UI_PEEK_PEEK_UTILS_H_
