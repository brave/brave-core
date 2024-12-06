/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.listener

import org.chromium.chrome.browser.playlist.model.PlaylistOptionsModel

interface PlaylistOptionsListener {
    fun onPlaylistOptionClicked(playlistOptionsModel: PlaylistOptionsModel) {}
}
