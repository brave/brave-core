/**
 * Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist;

import android.content.Context;
import android.content.Intent;

import org.chromium.chrome.browser.playlist.PlaylistHostActivity;

public class PlaylistUtils {
    public static final String TAG = "BravePlaylist";
    public static final String DEFAULT_PLAYLIST_ID = "default";
    public static final String PLAYLIST_NAME = "playlist_name";
    public static final String PLAYLIST_ID = "playlist_id";

    public static void openPlaylistActivity(Context context, String playlistId) {
        Intent playlistActivityIntent = new Intent(context, PlaylistHostActivity.class);
        playlistActivityIntent.putExtra(PLAYLIST_ID, playlistId);
        playlistActivityIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        context.startActivity(playlistActivityIntent);
    }
}
