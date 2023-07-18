/*
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.playlist.kotlin.enums

enum class PlaylistItemEventEnum {
    NONE,
    ITEM_ADDED,             // a new playlist item added but not ready state
    ITEM_THUMBNAIL_READY,    // Thumbnail ready to use for playlist
    ITEM_THUMBNAIL_FAILED,   // Failed to fetch thumbnail
    ITEM_CACHED,            // The item is cached in local storage
    ITEM_DELETED,           // An item deleted
    ITEM_UPDATED,           // An item's properties have been changed
    ITEM_ABORTED,           // Aborted during the creation process
    ITEM_LOCAL_DATA_REMOVED,  // Local data removed

    UPDATED,
}
