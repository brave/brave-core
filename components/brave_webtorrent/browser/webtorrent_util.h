/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WEBTORRENT_BROWSER_WEBTORRENT_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_WEBTORRENT_BROWSER_WEBTORRENT_UTIL_H_

class GURL;

namespace content {
class BrowserContext;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace net {
class HttpResponseHeaders;
}

namespace webtorrent {

bool IsWebtorrentEnabled(content::BrowserContext* browser_context);
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);
bool IsWebtorrentURL(const GURL& url);
bool IsTorrentFile(const GURL& url, const net::HttpResponseHeaders* headers);
bool TorrentURLMatched(const GURL& url);

}  // namespace webtorrent

#endif  // BRAVE_COMPONENTS_BRAVE_WEBTORRENT_BROWSER_WEBTORRENT_UTIL_H_
