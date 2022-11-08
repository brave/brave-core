// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// components
import { LoadingSkeleton } from '../../../components/shared'

// styles
import {
  MagnifyButton,
  ImageWrapper,
  Image
} from './nft-image.styles'
import { MediaSkeletonWrapper } from '../nft-multimedia/nft-multimedia.styles'

interface Props {
  imageUrl: string
  mimeType: string
  onMagnify: () => void
}
export const NftImage = (props: Props) => {
  const { imageUrl, onMagnify } = props

  // state
  const [isImageLoaded, setIsImageLoaded] = React.useState<boolean>()

  return (
    <>
      <ImageWrapper isLoading={!isImageLoaded}>
        <Image src={imageUrl} onLoad={() => setIsImageLoaded(true)} />
        <MagnifyButton onClick={onMagnify} />
      </ImageWrapper>
      {!isImageLoaded &&
        <LoadingSkeleton wrapper={MediaSkeletonWrapper} />
      }
    </>
  )
}
