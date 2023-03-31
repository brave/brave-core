// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { OverallPinningStatus, useNftPin } from '../../../common/hooks/nft-pin'

// styles
import { GifWrapper, Ipfs, IpfsUploading, StatusGif, StyledWrapper } from './nft-pinning-status-animation.style'
import UploadingGif from '../../../assets/svg-icons/nft-ipfs/uploading.gif'
import SuccessGif from '../../../assets/svg-icons/nft-ipfs/success.gif'

interface Props {
  size: string | undefined
  status: OverallPinningStatus
  isAutopinEnabled: boolean
}

export const NftPinningStatusAnimation = ({ size, status, isAutopinEnabled }: Props) => {
  const { pinnableNftsCount } = useNftPin()

  return (
    <StyledWrapper size={size || '30px'}>
      {(!isAutopinEnabled || pinnableNftsCount === 0) ? (
        <Ipfs size={size} />
      ) : status === OverallPinningStatus.PINNING_IN_PROGRESS ? (
        <GifWrapper>
          <StatusGif src={UploadingGif} />
          <IpfsUploading />
        </GifWrapper>
      ) : (
        status === OverallPinningStatus.PINNING_FINISHED && (
          <GifWrapper>
            <StatusGif src={SuccessGif} />
          </GifWrapper>
        )
      )}
    </StyledWrapper>
  )
}
