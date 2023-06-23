// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import { RouteComponentProps } from 'react-router-dom'

import * as PlaylistMojo from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'
import * as States from '../reducers/states'
import VideoFrame from './videoFrame'
import PlaylistItem from './playlistItem'

interface MatchParams {
  playlistId: string
}

export default function PlaylistPlayer ({
  match
}: RouteComponentProps<MatchParams>) {
  const playlist = useSelector<
    States.ApplicationState,
    PlaylistMojo.Playlist | undefined
  >(applicationState =>
    applicationState.playlistData?.lists.find(
      e => e.id === match.params.playlistId
    )
  )

  const playing = useSelector<States.ApplicationState, boolean>(
    applicationState =>
      !!applicationState.playlistData?.lastPlayerState?.playing
  )

  return (
    <>
      <VideoFrame playing={playing} />
      {playlist?.items.map(item => (
        <PlaylistItem
          id={item.id}
          name={item.name}
          thumbnailUrl={item.thumbnailPath.url}
          onClick={() => {}}
        />
      ))}
    </>
  )
}
