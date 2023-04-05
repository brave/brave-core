// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { BraveWallet } from '../../../constants/types'

// hooks
import { useNftPin } from '../../../common/hooks/nft-pin'

// styles
import { GifWrapper, Ipfs, IpfsUploading, StatusGif, StyledWrapper } from './nft-pinning-status-animation.style'
import UploadingDarkGif from '../../../assets/svg-icons/nft-ipfs/uploading-dark.gif'
import UploadingLightGif from '../../../assets/svg-icons/nft-ipfs/uploading-light.gif'
import SuccessDarkGif from '../../../assets/svg-icons/nft-ipfs/success-dark.gif'
import SuccessLightGif from '../../../assets/svg-icons/nft-ipfs/success-light.gif'

interface Props {
  size: string | undefined
  status: BraveWallet.TokenPinStatusCode
  isAutopinEnabled: boolean
}

export const NftPinningStatusAnimation = ({ size, status, isAutopinEnabled }: Props) => {
  const { STATUS_PINNING_IN_PROGRESS, STATUS_PINNED } = BraveWallet.TokenPinStatusCode
  const { pinnableNftsCount } = useNftPin()

  return (
    <StyledWrapper
      size={
        status === STATUS_PINNING_IN_PROGRESS || status === STATUS_PINNED
          ? '30px'
          : size || '14px'
      }
    >
      {!isAutopinEnabled || pinnableNftsCount === 0 ? (
        <Ipfs size={size} />
      ) : status === STATUS_PINNING_IN_PROGRESS ? (
        <GifWrapper>
          <StatusGif
            src={
              window.matchMedia('(prefers-color-scheme: dark)').matches
                ? UploadingDarkGif
                : UploadingLightGif
            }
          />
          <IpfsUploading />
        </GifWrapper>
      ) : (
        status === STATUS_PINNED && (
          <GifWrapper>
            <StatusGif
              src={
                window.matchMedia('(prefers-color-scheme: dark)').matches
                  ? SuccessDarkGif
                  : SuccessLightGif
              }
            />
          </GifWrapper>
        )
      )}
    </StyledWrapper>
  )
}
