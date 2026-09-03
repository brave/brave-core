/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_CONSTANTS_H_
#define BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_CONSTANTS_H_

#include "net/base/schemeful_site.h"
#include "url/gurl.h"

namespace playlist {

inline constexpr char kDefaultPlaylistID[] = "default";

// Playlist V2 intentionally keeps the existing detector-based implementation
// for YouTube while its SABR transport remains unsupported. Match the legacy
// site-specific detector exactly: HTTPS youtube.com and its subdomains.
inline bool IsYoutubeLegacyPlaylistSite(const GURL& url) {
  return net::SchemefulSite(url) ==
         net::SchemefulSite(GURL("https://youtube.com"));
}

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_CONSTANTS_H_
