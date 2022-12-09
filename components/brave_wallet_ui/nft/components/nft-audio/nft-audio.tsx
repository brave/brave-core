// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  Audio,
  PlaybackWrapper,
  PosterImage
  // PosterImageWrapper
} from './nft-audio.style'

interface Props {
  posterUrl?: string
  audioUrl?: string
}

export const NftAudio = (props: Props) => {
  const { posterUrl, audioUrl } = props

  return (
    <PlaybackWrapper>
      {posterUrl &&
        <PosterImage src={posterUrl}/>
      }
      <Audio
        controls
        controlsList="nodownload"
        loop
        src={audioUrl}
      />
    </PlaybackWrapper>
  )
}
