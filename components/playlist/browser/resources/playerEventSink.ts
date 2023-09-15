// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useHistory } from 'react-router'
import { getPlaylistAPI } from './api/api'
import { getPlaylistActions } from './api/getPlaylistActions'
import { PlayerEventsPayload } from './api/playerEventsNotifier'
import { types } from './constants/playlist_types'
import { History } from 'history'
import { useEffect } from 'react'

let history: History | undefined

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
    
    case types.PLAYLIST_GO_BACK_TO_CURRENTLY_PLAYING_FOLDER: {
      if (history) {
        const {currentList, currentItem } = payload.data
        history.push(`/playlist/${currentList.id}#${currentItem.id}`)
      }
      break;
    }

    case types.PLAYLIST_OPEN_SOURCE_PAGE: {
      window.open(
        payload.data.pageSource.url,
        '_blank',
        'noopener,noreferrer'
      )
    }
  }
}

export function useHistorySynchronization () {
  const h = useHistory()
  useEffect(() => {
    history = h
    return () => history = undefined
  }, [h])
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

