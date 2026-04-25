/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Types for Player
export enum types {
  PLAYLIST_ITEM_SELECTED = '@@playlist/PLAYLIST_ITEM_SELECTED',

  PLAYER_STARTED_PLAYING_ITEM = '@@playlist/PLAYER_STARTED_PLAYING_ITEM',

  PLAYER_STOPPED_PLAYING_ITEM = '@@playlist/PLAYER_STOPPED_PLAYING_ITEM',

  PLAYER_PLAY_NEXT_ITEM = '@@playlist/PLAYER_PLAY_NEXT_ITEM',

  PLAYER_PLAY_PREVIOUS_ITEM = '@@playlist/PLAYER_PLAY_PREVIOUS_ITEM',

  PLAYER_TOGGLE_SHUFFLE = '@@playlist/PLAYER_TOGGLE_SHUFFLE',

  PLAYER_ADVANCE_LOOP_MODE = '@@playlist/PLAYER_ADVANCE_LOOP_MODE',

  PLAYER_TOGGLE_AUTO_PLAY = '@@playlist/PLAYER_TOGGLE_AUTO_PLAY',

  SELECTED_PLAYLIST_UPDATED = '@@playlist/SELECTED_PLAYLIST_UPDATED',

  UNLOAD_PLAYLIST = '@@playlist/UNLOAD_PLAYLIST'
}
