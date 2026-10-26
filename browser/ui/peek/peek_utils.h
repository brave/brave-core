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

namespace url {
class Origin;
}  // namespace url

namespace peek {

// Only http(s) pages can be shown in Peek.
bool IsPeekableURL(const GURL& url);

// Whether a link in a frame with `origin` can be shown in Peek. Links from
// browser pages, files and opaque origins can't, since the load needs an
// initiator for SameSite cookies and fetch metadata.
bool IsPeekableInitiator(const url::Origin& origin);

// Whether `params` come from a Shift+click on a link in a page.
bool IsShiftClickOpen(const content::OpenURLParams& params);

// Whether `params` can be loaded in Peek: an http(s) URL from an http(s) page,
// so the load is treated like a link click rather than a browser-initiated
// navigation.
bool CanOpenInPeek(const content::OpenURLParams& params);

}  // namespace peek

#endif  // BRAVE_BROWSER_UI_PEEK_PEEK_UTILS_H_
