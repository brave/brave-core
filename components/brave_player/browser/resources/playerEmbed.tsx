/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { render } from 'react-dom'
import styled from 'styled-components'

// For now we only supports youtube.
// Usages: EmbedURL`youtube${videoId}`
export function EmbedURL (source: string, videoId: string) {
  if (source === 'youtube')
    return `https://www.youtube-nocookie.com/embed/${videoId}?iv_load_policy=1&autoplay=1&rel=0&modestbranding=1`

  return ''
}

const EmbedContainer = styled.div`
  display: flex;
  justify-content: center;
  align-items: center;
  height: 100%;
`

const StyledFrame = styled.iframe`
  width: 80%;
  height: 80%;
  border: none;
`

export function getFrameSrc (pathname: string) {
  const paths = pathname.split('/')
  if (paths.length < 3) {
    return ''
  }

  const [, service, videoId] = paths

  return EmbedURL(service, videoId)
}

function PlayerEmbed () {
  return (
    <EmbedContainer>
      <StyledFrame
        src={getFrameSrc(window.location.pathname)}
        allow='accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share;'
        allowFullScreen
      />
    </EmbedContainer>
  )
}

function initialize () {
  render(<PlayerEmbed />, document.getElementById('root'))
}

document.addEventListener('DOMContentLoaded', initialize)
