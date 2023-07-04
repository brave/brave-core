// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import styled from 'styled-components'

import { ApplicationState } from 'components/playlist/browser/resources/reducers/states'
import { PlaylistItem } from 'gen/brave/components/playlist/common/mojom/playlist.mojom.m.js'

import { getAllActions } from '../api/getAllActions'
import PlayerControls from './playerControls'
import { color, font } from '@brave/leo/tokens/css'
import PlayerSeeker from './playerSeeker'

const StyledVideo = styled.video`
  width: 100vw;
  aspect-ratio: 16 / 9;
`

const PlayerContainer = styled.div`
  display: flex;
  flex-direction: column;
  user-select: none; // In order to drag-and-drop on the seeker works well.
`

const ControlsContainer = styled.div`
  display: flex;
  flex-direction: column;
  padding: 24px 16px;
  gap: 8px;
`

const StyledTitle = styled.span`
  color: ${color.text.primary};
  font: ${font.primary.xSmall.regular};
`

const StyledPlayerControls = styled(PlayerControls)`
  padding-top: 8px;
`

export default function Player () {
  const currentItem = useSelector<ApplicationState, PlaylistItem | undefined>(
    applicationState => applicationState.playerState?.currentItem
  )

  const playing = useSelector<ApplicationState, PlaylistItem | undefined>(
    applicationState => applicationState.playerState?.currentItem
  )

  const [videoElement, setVideoElement] =
    React.useState<HTMLVideoElement | null>(null)

  React.useEffect(() => {
    // Route changes in props to parent frame.
    // Note that we are checking this condition as per SonarCloud.
    if (location.protocol.startsWith('chrome-untrusted:')) {
      window.parent.postMessage(
        {
          currentItem,
          playing
        },
        'chrome-untrusted://playlist'
      )
    }
  }, [playing])

  return (
    <PlayerContainer>
      <StyledVideo
        ref={setVideoElement}
        autoPlay
        onPlay={() => getAllActions().playerStartedPlayingItem(currentItem)}
        onPause={() => getAllActions().playerStoppedPlayingItem(currentItem)}
        onEnded={() => getAllActions().playerStoppedPlayingItem(currentItem)}
        src={currentItem?.mediaPath.url}
      />
      <ControlsContainer>
        <StyledTitle>{currentItem?.name}</StyledTitle>
        <PlayerSeeker videoElement={videoElement} />
        <StyledPlayerControls videoElement={videoElement} />
      </ControlsContainer>
    </PlayerContainer>
  )
}
