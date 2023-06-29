// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

interface Props {
 playing: boolean
};

const StyledVideoFrame = styled.iframe`
  width: 100vw;
  border: none;
`

export default function videoFrame ({ playing }: Props) {
    return (
      <StyledVideoFrame id="player" src="chrome-untrusted://playlist-player" allow="autoplay" scrolling="no" sandbox="allow-scripts allow-same-origin" data-playing={playing} />
    )
}
