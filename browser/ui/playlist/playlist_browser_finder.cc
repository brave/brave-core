/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/playlist/playlist_browser_finder.h"

namespace playlist {

#if !defined(TOOLKIT_VIEWS)
Browser* FindBrowserForPlaylistWebUI(content::WebContents* web_contents) {
  return nullptr;
}
#endif  // !defined(TOOLKIT_VIEWS)

}  // namespace playlist
