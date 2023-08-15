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

void ShowCreatePlaylistDialog(content::WebContents* contents);
void ShowRemovePlaylistDialog(content::WebContents* contents,
                              const std::string& playlist_id);
void ShowMoveItemsDialog(content::WebContents* contents,
                         const std::string& playlist_id,
                         const std::vector<std::string>& items);

}  // namespace playlist

#endif  // BRAVE_BROWSER_UI_PLAYLIST_PLAYLIST_DIALOGS_H_
