/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/playlist/playlist_dialogs.h"

#include "base/notreached.h"

namespace playlist {

#if !defined(TOOLKIT_VIEWS)

void ShowCreatePlaylistDialog(content::WebContents* web_contents) {
  NOTREACHED_IN_MIGRATION();
}

void ShowRemovePlaylistDialog(content::WebContents* web_contents,
                              const std::string& playlist_id) {
  NOTREACHED_IN_MIGRATION();
}

void ShowMoveItemsDialog(content::WebContents* contents,
                         const std::string& playlist_id,
                         const std::vector<std::string>& items) {
  NOTREACHED_IN_MIGRATION();
}

void ShowPlaylistSettings() {
  NOTREACHED_IN_MIGRATION();
}

void ClosePanel() {
  NOTREACHED_IN_MIGRATION();
}
// #else functions are defined in playlist_action_dialogs.cc
#endif

}  // namespace playlist
