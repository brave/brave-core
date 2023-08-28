// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled, { css } from 'styled-components'

import { color, effect } from '@brave/leo/tokens/css'

import { playlistControlsAreaHeight } from '../constants/style'

interface Props {
  visible: boolean
  isMiniPlayer: boolean
}

const StyledVideoFrame = styled.iframe<Props>`
  ${playlistControlsAreaHeight}

  width: 100vw;
  border: none;
  ${p =>
    p.isMiniPlayer
      ? css`
          position: fixed;
          bottom: 0;
          height: var(--player-controls-area-height);
          z-index: 1;
          border-top: 1px solid ${color.divider.subtle};
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

export default function VideoFrame (props: Props) {
  return (
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
      {...props}
    />
  )
}
