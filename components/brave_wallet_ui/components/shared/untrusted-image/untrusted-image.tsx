// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import {
  httpifyIpfsUrl
  // ,
  // stripERC20TokenImageURL
} from '../../../utils/string-utils'

// style
import {
  LoadingOverlay,
  UntrustedImageIframe,
  LoadIcon
} from './untrusted-image.styles'

interface Props {
  src?: string
  responsive?: boolean
  width?: string | number
  height?: string | number
}

export const UntrustedImage = React.memo(({
  src,
  responsive,
  height,
  width
}: Props) => {
  // state
  const [isLoaded, setIsLoaded] = React.useState<boolean>(false)

  // computed
  const imgURL = httpifyIpfsUrl(src)

  // render
  return <>
    <LoadingOverlay isLoading={!isLoaded}>
      <LoadIcon />
    </LoadingOverlay>

    <UntrustedImageIframe
      src={`chrome-untrusted://image-display/?targetUrl=${imgURL}`}
      sandbox="allow-scripts"
      responsive={responsive}
      width={width}
      height={height}
      onLoad={() => setIsLoaded(true)}
      loading='lazy'
    />
  </>
})
