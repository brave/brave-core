/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export const enum types {
  PLAYLIST_LOADED = '@@playlist/PLAYLIST_LOADED',

  PLAYLIST_UPDATED = '@@playlist/PLAYLIST_UPDATED',

  PLAYLIST_PLAYER_STATE_CHANGED = '@@playlist/PLAYLIST_PLAYER_STATE_CHANGED',

  PLAYLIST_CACHING_PROGRESS_CHANGED = '@@playlist/PLAYLIST_CACHING_PROGRESS_CHANGED',

  PLAYLIST_SET_EDIT_MODE = '@@playlist/PLAYLIST_EDIT_MODE',

  PLAYLIST_LAST_PLAYED_POSITION_OF_CURRENT_ITEM_CHANGED = '@@playlist/PLAYLIST_LAST_PLAYED_POSITION_CHANGED',

  PLAYLIST_GO_BACK_TO_CURRENTLY_PLAYING_FOLDER = '@@playlist/PLAYLIST_GO_BACK_TO_CURRENTLY_PLAYING_FOLDER',

  PLAYLIST_OPEN_SOURCE_PAGE = '@@playlist/PLAYLIST_OPEN_SOURCE_PAGE',

  PLAYLIST_PLAYER_FAILED_TO_PLAY_ITEM = '@@playlist/PLAYLIST_PLAYER_FAILED_TO_PLAY_ITEM',

  PLAYLIST_SHOULD_SHOW_ADD_MEDIA_FROM_PAGE_CHANGED = '@@playlist/PLAYLIST_SHOULD_SHOW_ADD_MEDIA_FROM_PAGE_CHANGED'

  // TODO(sko) Need more actions for each events
}
