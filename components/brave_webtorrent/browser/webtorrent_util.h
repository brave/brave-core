/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WEBTORRENT_BROWSER_WEBTORRENT_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_WEBTORRENT_BROWSER_WEBTORRENT_UTIL_H_

namespace content {
class BrowserContext;
}

namespace webtorrent {

bool IsWebtorrentEnabled(content::BrowserContext* browser_context);

}  // namespace webtorrent

#endif  // BRAVE_COMPONENTS_BRAVE_WEBTORRENT_BROWSER_WEBTORRENT_UTIL_H_
