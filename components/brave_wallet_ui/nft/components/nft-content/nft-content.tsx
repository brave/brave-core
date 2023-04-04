// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import { Image, LoadingOverlay, LoadIcon, ImageWrapper } from './nft-content-styles'

// types
import { BraveWallet, NFTMetadataReturnType } from '../../../constants/types'
import { DisplayMode } from '../../nft-ui-messages'
import { PinningStatusType } from '../../../page/constants/action_types'

// components
import Placeholder from '../../../assets/svg-icons/nft-placeholder.svg'
import { NftMultimedia } from '../nft-multimedia/nft-multimedia'

interface Props {
  isLoading?: boolean
  displayMode?: DisplayMode
  selectedAsset?: BraveWallet.BlockchainToken
  nftMetadata?: NFTMetadataReturnType
  nftMetadataError?: string
  tokenNetwork?: BraveWallet.NetworkInfo
  imageUrl?: string
  nftPinningStatus?: PinningStatusType | undefined
  imageIpfsUrl?: string
}

export const NftContent = (props: Props) => {
  const {
    isLoading,
    nftMetadata,
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
      { displayMode === 'details' && nftMetadata &&
        <NftMultimedia nftMetadata={nftMetadata} />
      }
    </>
  )
}
