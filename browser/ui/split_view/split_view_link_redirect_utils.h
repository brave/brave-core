/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_LINK_REDIRECT_UTILS_H_
#define BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_LINK_REDIRECT_UTILS_H_

namespace content {
struct Referrer;
class WebContents;
}  // namespace content

class GURL;

namespace split_view {

// Manages redirecting navigations from the left pane in a split view to the
// right pane when the split view is linked. This shared logic is used by both
// SplitViewLinkNavigationThrottle (for normal navigations) and Browser's
// AddNewContents (for target="_blank" links).
bool MaybeRedirectToRightPane(content::WebContents* source,
                              const GURL& url,
                              const content::Referrer& referrer);
}  // namespace split_view

#endif  // BRAVE_BROWSER_UI_SPLIT_VIEW_SPLIT_VIEW_LINK_REDIRECT_UTILS_H_
