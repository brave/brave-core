// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled, { css } from 'styled-components'

import { color, effect, spacing } from '@brave/leo/tokens/css'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'

import { playlistControlsAreaHeight } from '../constants/style'
import postMessageToPlayer from '../api/playerApi'
import { types } from '../constants/player_types'

interface Props {
  visible: boolean
  isMiniPlayer: boolean
}

const VideoFrameContainer = styled.div<Props>`
  position: relative;
  width: 100vw;

  ${playlistControlsAreaHeight}
  ${p =>
    p.isMiniPlayer
      ? css`
          position: fixed;
          bottom: 0;
          height: var(--player-controls-area-height);
          z-index: 1;
        `
      : css`
          // 16:9 aspect ratio for video and fixed height for the controls area
          height: calc(56vw + var(--player-controls-area-height));
          margin-bottom: 8px;
          box-shadow: ${effect.elevation['02']};
        `}

  ${({ visible }) =>
    !visible &&
    css`
      display: none;
    `}
`

const StyledCloseButton = styled(Button)`
  position: absolute;
  margin: ${spacing.s};
  right: 0;
`

function CloseButton () {
  return (
    <StyledCloseButton
      kind='plain-faint'
      size='tiny'
      onClick={() => {
        postMessageToPlayer({ actionType: types.UNLOAD_PLAYLIST })
      }}
    >
      <Icon name='close'></Icon>
    </StyledCloseButton>
  )
}

const StyledVideoFrame = styled.iframe<Pick<Props, 'isMiniPlayer'>>`
  position: absolute;
  width: 100vw;
  height: 100%;
  border: none;
  ${p =>
    p.isMiniPlayer &&
    css`
      border-top: 1px solid ${color.divider.subtle};
    `}
`

export default function VideoFrame (props: Props) {
  return (
    <VideoFrameContainer {...props}>
      <StyledVideoFrame
        id='player'
        src={
          location.protocol === 'chrome-untrusted:'
            ? 'chrome-untrusted://playlist-player'
            : 'iframe.html?id=playlist-components--video-player'
        }
        allow='autoplay; fullscreen;'
        scrolling='no'
        sandbox='allow-scripts allow-same-origin'
        isMiniPlayer={props.isMiniPlayer}
      />
      {props.isMiniPlayer && <CloseButton />}
    </VideoFrameContainer>
  )
}
