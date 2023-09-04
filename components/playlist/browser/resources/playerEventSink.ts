// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getPlaylistAPI } from './api/api'
import { getPlaylistActions } from './api/getPlaylistActions'
import { PlayerEventsPayload } from './api/playerEventsNotifier'
import { types } from './constants/playlist_types'

function handlePlayerEvents (payload: PlayerEventsPayload) {
  switch (payload.type) {
    case types.PLAYLIST_PLAYER_STATE_CHANGED:
      getPlaylistActions().playerStateChanged(payload.data)
      break

    case types.PLAYLIST_LAST_PLAYED_POSITION_OF_CURRENT_ITEM_CHANGED:
      getPlaylistAPI().updateItemLastPlayedPosition(
        payload.data.id,
        payload.data.lastPlayedPosition
      )
      break
  }
}

// Used to mirror state of Player from Playlist side.
export default function startReceivingPlayerEvents () {
  window.onmessage = e => {
    if (e.origin !== 'chrome-untrusted://playlist-player') {
      console.error(`Invalid origin: ${e.origin}`)
      return
    }

    handlePlayerEvents(e.data)
  }
}
