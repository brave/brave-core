// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { PlayerState } from 'components/playlist/browser/resources/reducers/states'

import { getPlaylistActions } from './api/getPlaylistActions'

// Used to mirror state of Player from Playlist side.
export default function startReceivingPlayerEvents () {
  window.onmessage = e => {
    if (e.origin !== 'chrome-untrusted://playlist-player') {
      console.error(`Invalid origin: ${e.origin}`)
      return
    }

    getPlaylistActions().playerStateChanged(e.data as PlayerState)
  }
}
