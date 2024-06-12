/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_TABS_TABS_UTIL_H_
#define BRAVE_BROWSER_BRAVE_ADS_TABS_TABS_UTIL_H_

class SessionID;

namespace content {
class WebContents;
}  // namespace content

namespace net {
class HttpResponseHeaders;
}  // namespace net

namespace brave_ads {

SessionID GetTabIdFromWebContents(content::WebContents* const web_contents);

bool HttpResponseHasErrorCode(
    const net::HttpResponseHeaders* const response_headers);

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_TABS_TABS_UTIL_H_
