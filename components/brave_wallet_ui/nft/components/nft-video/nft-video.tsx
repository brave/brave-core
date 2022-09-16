// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import * as React from 'react'

import {
  NftVideoWrapper,
  Video
} from './nft-video.styles'

interface Props {
  videoMimeType: string
  videoUrl: string
  posterUrl?: string
}

export const NftVideo = (props: Props) => {
  const {
    videoMimeType,
    videoUrl,
    posterUrl
  } = props
  return (
    <NftVideoWrapper>
      <Video
        autoPlay
        controls
        controlsList="nodownload"
        loop
        playsInline
        poster={posterUrl || ''}
        preload="metadata"
        muted
      >
        <source type={videoMimeType} src={videoUrl}/>
      </Video>
    </NftVideoWrapper>
  )
}
