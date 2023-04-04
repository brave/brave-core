// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { OverallPinningStatus, useNftPin } from '../../../common/hooks/nft-pin'

// styles
import { GifWrapper, Ipfs, IpfsUploading, StatusGif, StyledWrapper } from './nft-pinning-status-animation.style'
import UploadingDarkGif from '../../../assets/svg-icons/nft-ipfs/uploading-dark.gif'
import UploadingLightGif from '../../../assets/svg-icons/nft-ipfs/uploading-light.gif'
import SuccessDarkGif from '../../../assets/svg-icons/nft-ipfs/success-dark.gif'
import SuccessLightGif from '../../../assets/svg-icons/nft-ipfs/success-light.gif'

interface Props {
  size: string | undefined
  status: OverallPinningStatus
  isAutopinEnabled: boolean
}

export const NftPinningStatusAnimation = ({ size, status, isAutopinEnabled }: Props) => {
  const { pinnableNftsCount } = useNftPin()

  return (
    <StyledWrapper
      size={
        status === OverallPinningStatus.PINNING_IN_PROGRESS || status === OverallPinningStatus.PINNING_FINISHED
          ? '30px'
          : size || '14px'
      }
    >
      {!isAutopinEnabled || pinnableNftsCount === 0 ? (
        <Ipfs size={size} />
      ) : status === OverallPinningStatus.PINNING_IN_PROGRESS ? (
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
        status === OverallPinningStatus.PINNING_FINISHED && (
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
