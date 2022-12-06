// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { types } from '../constants/playlist_types'

function getPlayerWindow () {
  return (document.getElementById('player') as HTMLIFrameElement)?.contentWindow
}

export type PlayerMessagePayload = {
  actionType: types
  data: any
}

export default function postMessageToPlayer (payload: PlayerMessagePayload) {
  const playerWindow = getPlayerWindow()
  if (!playerWindow) {
    console.error('Couldn\'t find player window')
    return
  }

  playerWindow.postMessage(payload, 'chrome-untrusted://playlist-player')
}
