// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { PlaylistItem } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m'
import { types } from '../constants/playlist_types'
import { PlayerState } from '../reducers/states'
export { types as PlaylistTypes } from '../constants/playlist_types'

type PlayerStateChangedPayload = {
  data: PlayerState
}

type LastPlayedPositionChangedPayload = {
  data: PlaylistItem
}

export type PlayerEventsPayload =
  | ({
      type: types.PLAYLIST_PLAYER_STATE_CHANGED
    } & PlayerStateChangedPayload)
  | ({
      type: types.PLAYLIST_LAST_PLAYED_POSITION_OF_CURRENT_ITEM_CHANGED
    } & LastPlayedPositionChangedPayload)

export function notifyEventsToTopFrame (playerState: PlayerEventsPayload) {
  if (location.protocol.startsWith('chrome-untrusted:')) {
    window.parent.postMessage(playerState, 'chrome-untrusted://playlist')
  }
}
