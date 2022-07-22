// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// components
import { Image, LoadingOverlay, LoadIcon } from './nft-content-styles'
import { NftDetails } from '../nft-details/nft-details'

// utils
import { BraveWallet, NFTMetadataReturnType } from '../../../constants/types'

interface Props {
  isLoading?: boolean
  selectedAsset?: BraveWallet.BlockchainToken
  nftMetadata?: NFTMetadataReturnType
  tokenNetwork?: BraveWallet.NetworkInfo
  imageUrl?: string
}

export const NftContent = (props: Props) => {
  const {
    isLoading,
    selectedAsset,
    imageUrl
  } = props

  return (
    <>
      {imageUrl
        ? <Image
          src={imageUrl}
        />
        : <>
          {isLoading &&
            <LoadingOverlay isLoading={isLoading}>
              <LoadIcon />
            </LoadingOverlay>
          }
          {selectedAsset &&
            <NftDetails
              {...props}
              selectedAsset={selectedAsset}
            />
          }
        </>
      }
    </>
  )
}
