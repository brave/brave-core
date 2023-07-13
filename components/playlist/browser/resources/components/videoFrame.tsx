// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

interface Props {
  playing: boolean
}

const StyledVideoFrame = styled.iframe`
  width: 100vw;
  // 16:9 aspect ratio for video and fixed height for the controls area
  height: calc(56vw + 160px);
  border: none;
`

export default function VideoFrame ({ playing }: Props) {
  return (
    <StyledVideoFrame
      id='player'
      src='chrome-untrusted://playlist-player'
      allow='autoplay'
      scrolling='no'
      sandbox='allow-scripts allow-same-origin'
      data-playing={playing}
    />
  )
}
