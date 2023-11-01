// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// styles
import { Image } from './nft-image.styles'

interface ImageLoaderProps {
  src: string
  width?: string
  height?: string
  lazyLoad?: 'lazy' | 'eager'
  onLoaded?: () => void
}

export const ImageLoader = ({
  src,
  width,
  height,
  lazyLoad,
  onLoaded
}: ImageLoaderProps) => {
  const [loaded, setLoaded] = React.useState(false)

  const handleImageLoaded = () => {
    setLoaded(true)
    if (onLoaded) onLoaded()
  }

  return (
    <>
      {loaded && (
        <Image
          src={src}
          customWidth={width}
          customHeight={height}
        />
      )}
      <img
        src={src}
        style={{ display: 'none' }}
        loading={lazyLoad}
        onLoad={handleImageLoaded}
      />
    </>
  )
}
