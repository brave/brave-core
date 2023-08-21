// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import styled from 'styled-components'

import { PlaylistItem } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

import {
  ApplicationState,
  PlayerState
} from 'components/playlist/browser/resources/reducers/states'
import { getPlayerActions } from '../api/getPlayerActions'
import PlayerControls from './playerControls'
import { color, font } from '@brave/leo/tokens/css'
import PlayerSeeker from './playerSeeker'
import { playlistControlsAreaHeight } from '../constants/style'

const StyledVideo = styled.video`
  width: 100vw;
  aspect-ratio: 16 / 9;
  margin-bottom: 8px;
`

const PlayerContainer = styled.div`
  width: 100vw;
  height: 100vh;
  display: flex;
  flex-direction: column-reverse;
  overflow: hidden;
  user-select: none; // In order to drag-and-drop on the seeker works well.
`

const ControlsContainer = styled.div`
  ${playlistControlsAreaHeight}
  height: var(--player-controls-area-height);

  display: flex;
  flex-direction: column;
  padding: 0px repeat(3, 16px);
  gap: 8px;
  flex-shrink: 0;
  justify-content: center;
`

const StyledTitle = styled.div`
  color: ${color.text.primary};
  font: ${font.primary.large.semibold};
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
`

const StyledPlayerControls = styled(PlayerControls)`
  padding-top: 8px;
`

// Route changes of PlayerState to parent frame.
function useStateRouter () {
  const playerState = useSelector<ApplicationState, PlayerState | undefined>(
    applicationState => applicationState.playerState
  )
  React.useEffect(() => {
    // Note that we are checking this condition as per SonarCloud.
    if (location.protocol.startsWith('chrome-untrusted:')) {
      window.parent.postMessage(playerState, 'chrome-untrusted://playlist')
    }
  }, [playerState])
}

export default function Player () {
  useStateRouter()

  const currentItem = useSelector<ApplicationState, PlaylistItem | undefined>(
    applicationState => applicationState.playerState?.currentItem
  )

  const [videoElement, setVideoElement] =
    React.useState<HTMLVideoElement | null>(null)

  React.useEffect(() => {
    if (videoElement && !videoElement.paused && !currentItem) {
      // This could happen when the current item was deleted. In this case,
      // we should pause the video first. Otherwise, video will keep playing
      // even we clear the src attribute.
      videoElement.pause()
    }
  }, [currentItem, videoElement])

  return (
    <PlayerContainer>
      <ControlsContainer>
        <StyledTitle>{currentItem?.name}</StyledTitle>
        <PlayerSeeker videoElement={videoElement} />
        <StyledPlayerControls videoElement={videoElement} />
      </ControlsContainer>
      <StyledVideo
        ref={setVideoElement}
        autoPlay
        onPlay={() => getPlayerActions().playerStartedPlayingItem(currentItem)}
        onPause={() => getPlayerActions().playerStoppedPlayingItem(currentItem)}
        onEnded={() => {
          getPlayerActions().playerStoppedPlayingItem(currentItem)
          getPlayerActions().playNextItem() // In case the current item is the last one, nothing will happen
        }}
        src={
          videoElement?.src === currentItem?.mediaSource.url
            ? currentItem?.mediaSource.url // We were already playing the item with the original source url instead of cached file
            : currentItem?.mediaPath.url
        }
      />
    </PlayerContainer>
  )
}
