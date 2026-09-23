/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_PLAYLIST_PLAYLIST_DIALOGS_H_
#define BRAVE_BROWSER_UI_PLAYLIST_PLAYLIST_DIALOGS_H_

#include <string>
#include <vector>

namespace content {
class WebContents;
}  // namespace content

namespace playlist {

// Definitions of these functions live in playlist_action_dialogs.cc as they
// are tookit(views) dependent.
void ShowCreatePlaylistDialog(content::WebContents* contents);
void ShowRemovePlaylistDialog(content::WebContents* contents,
                              const std::string& playlist_id);
void ShowMoveItemsDialog(content::WebContents* contents,
                         const std::string& playlist_id,
                         const std::vector<std::string>& items);
void ShowPlaylistSettings(content::WebContents* contents);
void ShowPlaylistAddBubble(content::WebContents* contents);
void ClosePanel(content::WebContents* contents);

}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_PLAYLIST_PLAYLIST_DIALOGS_H_
