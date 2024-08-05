// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { getPlaylistActions } from './api/getPlaylistActions'
import { getPlaylistAPI } from './api/api'

async function getInitialData () {
  return getPlaylistAPI().getAllPlaylists()
}

export default function wireApiEventsToStore () {
  // Get initial data and dispatch to store
  getInitialData()
    .then(initialData => {
      getPlaylistActions().playlistLoaded(initialData.playlists)

      // TODO(sko): Add proper event listeners for changes to playlist.
      const api = getPlaylistAPI()
      api.addEventListener(e => {
        getInitialData().then(data => {
          getPlaylistActions().playlistLoaded(data.playlists)
        })
      })

      api.addMediaCachingProgressListener(
        (id, totalBytes, receivedBytes, percentComplete, timeRemaining) => {
          getPlaylistActions().cachingProgressChanged({
            id,
            totalBytes,
            receivedBytes,
            percentComplete,
            timeRemaining
          })
        }
      )

      api.addPlaylistUpdatedListener(playlist =>
        getPlaylistActions().playlistUpdated(playlist)
      )


      api.shouldShowAddMediaFromPageUI().then(({result}) => {
        getPlaylistActions().shouldShowAddMediaFromPageChanged(result)

        api.addOnActiveTabChangedListener((shouldShowAddMediaFromPageUI) => {
          getPlaylistActions().shouldShowAddMediaFromPageChanged(shouldShowAddMediaFromPageUI)
        })
      })
    })
    .catch(e => {
      console.error('Playlist page fatal error:', e)
    })
}
