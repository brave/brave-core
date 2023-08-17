// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import { Image, ImageWrapper } from './nft-content-styles'

// types
import { NFTMetadataReturnType } from '../../../constants/types'
import { DisplayMode } from '../../nft-ui-messages'

// components
import Placeholder from '../../../assets/svg-icons/nft-placeholder.svg'
import { NftMultimedia } from '../nft-multimedia/nft-multimedia'
import { stripChromeImageURL } from '../../../utils/string-utils'

interface Props {
  isLoading?: boolean
  displayMode?: DisplayMode
  nftMetadata?: NFTMetadataReturnType
  imageUrl?: string
}

export const NftContent = (props: Props) => {
  const {
    nftMetadata,
    imageUrl,
    displayMode
  } = props

  const url = React.useMemo(() => {
    return stripChromeImageURL(imageUrl) ?? Placeholder
  }, [imageUrl])

  return (
    <>
      {url && displayMode === 'icon' &&
        <ImageWrapper>
          <Image
            src={url}
          />
        </ImageWrapper>
      }
      {displayMode === 'details' && nftMetadata &&
        <NftMultimedia nftMetadata={nftMetadata} />
      }
    </>
  )
}
