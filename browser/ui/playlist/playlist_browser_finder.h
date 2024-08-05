/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_PLAYLIST_PLAYLIST_BROWSER_FINDER_H_
#define BRAVE_BROWSER_UI_PLAYLIST_PLAYLIST_BROWSER_FINDER_H_

namespace content {
class WebContents;
}  // namespace content

class Browser;

namespace playlist {

// Returns the Browser that contains the given playlist web ui's WebContents.
// The toolkit(views) dependent implementation is in
// playlist_browser_finder_views.cc.
Browser* FindBrowserForPlaylistWebUI(content::WebContents* web_contents);

}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_PLAYLIST_PLAYLIST_BROWSER_FINDER_H_
