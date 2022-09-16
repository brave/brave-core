// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
import * as React from 'react'

import {
  MediaSkeletonWrapper,
  NftMediaIframe
} from '../nft-multimedia/nft-multimedia.styles'
import LoadingSkeleton from '../../../components/shared/loading-skeleton'

interface Props {
  iframeSrc: string
}
export const MediaIframe = (props: Props) => {
  const { iframeSrc } = props

  // state
  const [isLoading, setIsLoading] = React.useState<boolean>(true)

  return (
    <>
      {isLoading && <LoadingSkeleton wrapper={MediaSkeletonWrapper} />}
      <NftMediaIframe
        visible={!isLoading}
        src={iframeSrc}
        onLoad={() => setIsLoading(false)}
        sandbox="allow-scripts allow-popups allow-same-origin"
        allowFullScreen
      />
    </>
  )
}
