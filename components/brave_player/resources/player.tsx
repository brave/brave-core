/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import styled from 'styled-components'

const PlayerContainer = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  flex: 1 0 0;
  align-self: stretch;
  width: 100%;
  height: 100%;
`

const StyledFrame = styled.iframe`
  width: 100%;
  height: 100%;
  border: none;
`

function Player () {
  return (
    <PlayerContainer>
      <StyledFrame
        src={`chrome-untrusted://player-embed${location.pathname}`}
      />
    </PlayerContainer>
  )
}

function initialize () {
  render(<Player />, document.getElementById('root'))
}

document.addEventListener('DOMContentLoaded', initialize)
