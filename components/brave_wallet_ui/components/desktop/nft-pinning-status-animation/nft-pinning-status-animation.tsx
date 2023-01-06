// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet } from '../../../constants/types'

// styles
import { GifWrapper, Ipfs, IpfsUploading, StatusGif, StyledWrapper } from './nft-pinning-status-animation.style'
import UploadingGif from '../../../assets/svg-icons/nft-ipfs/uploading.gif'
import SuccessGif from '../../../assets/svg-icons/nft-ipfs/success.gif'

interface Props {
  size: string | undefined
  status: BraveWallet.TokenPinStatusCode
  displayMode: 'nft' | 'banner'
}

export const NftPinningStatusAnimation = ({ size, status, displayMode }: Props) => {
  return (
    <StyledWrapper size={size || '30px'}>
      {/* Banner starting state */}
      {status === BraveWallet.TokenPinStatusCode.STATUS_NOT_PINNED &&
        displayMode === 'banner' && <Ipfs />
      }

      {/* Uploading */}
      {status === BraveWallet.TokenPinStatusCode.STATUS_PINNING_IN_PROGRESS && (
        <GifWrapper>
          <StatusGif src={UploadingGif} />
          <IpfsUploading />
        </GifWrapper>
      )}

      {/* Success */}
      {status === BraveWallet.TokenPinStatusCode.STATUS_PINNED && (
          <GifWrapper>
            <StatusGif src={SuccessGif} />
          </GifWrapper>
        )}
    </StyledWrapper>
  )
}
