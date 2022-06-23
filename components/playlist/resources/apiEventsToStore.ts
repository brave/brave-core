// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { getAllActions } from './api/getAllActions'
import { getPlaylistAPI } from './api/api'

async function getInitialData () {
  return getPlaylistAPI().getAllPlaylists()
}

export default function wireApiEventsToStore () {
  // Get initial data and dispatch to store
  getInitialData()
      .then((initialData) => {
        getAllActions().playlistLoaded(initialData.playlists)

        // TODO: Add proper event listeners for changes to playlist
        getPlaylistAPI().addEventListener((e) => {
          getInitialData().then(data => { getAllActions().playlistLoaded(data.playlists) })
        })
      })
      .catch(e => { console.error('New Tab Page fatal error:', e) })
}
