// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getAllActions } from './api/getAllActions'
import { PlayerMessagePayload } from './api/playerApi'
import { types } from './constants/playlist_types'

export function handlePlayerMessage (payload: PlayerMessagePayload) {
  switch (payload.actionType) {
    case types.PLAYLIST_ITEM_SELECTED: {
      getAllActions().selectPlaylistItem(payload)
      break
    }
    case types.SELECTED_PLAYLIST_UPDATED: {
      getAllActions().selectedPlaylistUpdated(payload)
      break
    }
  }
}

export default function startReceivingAPIRequest () {
  window.onmessage = e => {
    if (e.origin !== 'chrome-untrusted://playlist') {
      console.error(`Invalid origin: ${e.origin}`)
      return
    }

    handlePlayerMessage(e.data)
  }
}
