// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import { Image, LoadingOverlay, LoadIcon, ImageWrapper } from './nft-content-styles'
import { NftDetails } from '../nft-details/nft-details'

// types
import { BraveWallet, NFTMetadataReturnType } from '../../../constants/types'
import { DisplayMode } from '../../nft-ui-messages'

import Placeholder from '../../../assets/svg-icons/placeholdr-image.svg'

interface Props {
  isLoading?: boolean
  displayMode?: DisplayMode
  selectedAsset?: BraveWallet.BlockchainToken
  nftMetadata?: NFTMetadataReturnType
  nftMetadataError?: string
  tokenNetwork?: BraveWallet.NetworkInfo
  imageUrl?: string
}

export const NftContent = (props: Props) => {
  const {
    isLoading,
    selectedAsset,
    imageUrl,
    displayMode
  } = props

  const url = React.useMemo(() => {
    return imageUrl ? imageUrl?.replace('chrome://image?', '') : Placeholder
  }, [imageUrl])

  if (isLoading) {
    return (
      <LoadingOverlay isLoading={isLoading}>
        <LoadIcon />
      </LoadingOverlay>
    )
  }

  return (
    <>
      {url && displayMode === 'icon' &&
        <ImageWrapper>
          <Image
            src={url}
          />
        </ImageWrapper>
      }
      {selectedAsset && displayMode === 'details' &&
        <NftDetails
          {...props}
          selectedAsset={selectedAsset}
        />
      }
    </>
  )
}
