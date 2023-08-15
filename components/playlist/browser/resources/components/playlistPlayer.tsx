// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import { RouteComponentProps, Redirect } from 'react-router-dom'

import VideoFrame from './videoFrame'
import PlaylistItem from './playlistItem'
import { ApplicationState, PlayerState, usePlaylist } from '../reducers/states'
import postMessageToPlayer from '../api/playerApi'
import { types } from '../constants/playlist_types'

interface MatchParams {
  playlistId: string
}

export default function PlaylistPlayer ({
  match
}: RouteComponentProps<MatchParams>) {
  const playlist = usePlaylist(match.params.playlistId)
  const lastPlayerState = useSelector<
    ApplicationState,
    PlayerState | undefined
  >(applicationState => applicationState.playlistData?.lastPlayerState)

  React.useEffect(() => {
    // When playlist updated and player is working, notify that the current
    // list has changed.
    if (playlist && lastPlayerState) {
      postMessageToPlayer({
        actionType: types.SELECTED_PLAYLIST_UPDATED,
        currentList: playlist
      })
    }
  }, [playlist])

  if (!playlist) {
    // After deleting a playlist from header, this could happen. In this case,
    // redirect to the index page.
    return <Redirect to='/' />
  }

  return (
    <>
      <VideoFrame playing={!!lastPlayerState?.currentItem} />
      {playlist?.items.map(item => (
        <PlaylistItem
          key={item.id}
          item={item}
          onClick={() =>
            postMessageToPlayer({
              actionType: types.PLAYLIST_ITEM_SELECTED,
              currentList: playlist,
              currentItem: item
            })
          }
        />
      ))}
    </>
  )
}
