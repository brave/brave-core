/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import styled from 'styled-components'

const EmbedContainer = styled.div`
  display: flex;
  justify-content: center;
  align-items: center;
  height: 100%;
`

const StyledFrame = styled.iframe`
  max-width: 1000px;
  width: 80%;
  height: 80%;
  border: none;
`

function getFrameSrc () {
  let paths = new URL(window.location.href).pathname.split('/')
  if (paths.length < 3) {
    return ''
  }

  if (!paths[0]) {
    // could be empty. Drop it.
    paths = paths.slice(1)
  }

  if (paths[0] === 'youtube') {
    return `https://www.youtube-nocookie.com/embed/${paths[1]}?iv_load_policy=1&autoplay=1&rel=0&modestbranding=1`
  }

  return ''
}

function PlayerEmbed () {
  return (
    <EmbedContainer>
      <StyledFrame
        src={getFrameSrc()}
        allow='autoplay; encrypted-media; picture-in-picture;'
        sandbox='allow-popups allow-scripts allow-same-origin allow-presentation'
        allowFullScreen
      />
    </EmbedContainer>
  )
}

function initialize () {
  render(<PlayerEmbed />, document.getElementById('root'))
}

document.addEventListener('DOMContentLoaded', initialize)
