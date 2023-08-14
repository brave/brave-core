/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

export const enum types {
  // Types for Playlist --------------------------------------------------------
  PLAYLIST_LOADED = '@@playlist/PLAYLIST_LOADED',

  PLAYLIST_SELECTED = '@@playlist/PLAYLIST_SELECTED',

  PLAYLIST_ITEM_SELECTED = '@@playlist/PLAYLIST_ITEM_SELECTED',

  PLAYLIST_PLAYER_STATE_CHANGED = '@@playlist/PLAYLIST_PLAYER_STATE_CHANGED',

  PLAYLIST_CACHING_PROGRESS_CHANGED = '@@playlist/PLAYLIST_CACHING_PROGRESS_CHANGED',

  // Types for Player ----------------------------------------------------------
  PLAYER_STARTED_PLAYING_ITEM = '@@playlist/PLAYER_STARTED_PLAYING_ITEM',

  PLAYER_STOPPED_PLAYING_ITEM = '@@playlist/PLAYER_STOPPED_PLAYING_ITEM'

  // TODO(sko) Need more actions for each events
}
