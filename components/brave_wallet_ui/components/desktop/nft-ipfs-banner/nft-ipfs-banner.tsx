// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// utils
import { useNftPin } from '../../../common/hooks/nft-pin'
import { useUnsafePageSelector } from '../../../common/hooks/use-safe-selector'
import { BraveWallet, WalletRoutes } from '../../../constants/types'
import { PageSelectors } from '../../../page/selectors'

// components
import { Row } from '../../shared/style'
import { NftPinningStatusAnimation } from '../nft-pinning-status-animation/nft-pinning-status-animation'

export type BannerStatus = 'start' | 'uploading' | 'success'

interface Props {
  onDismiss: () => void
}

// styles
import {
  StyledWrapper,
  Text,
  LearnMore,
  CloseButton
} from './nft-ipfs-banner.style'

export const NftIpfsBanner = ({ onDismiss }: Props) => {
  const history = useHistory()

  const { pinnableNftsCount, pinnableNfts } = useNftPin()
  console.log(pinnableNftsCount)

  // redux
  const nftsPinningStatus = useUnsafePageSelector(PageSelectors.nftsPinningStatus)

  const pinnedNftsCount = React.useMemo(() => {
    return Object.keys(nftsPinningStatus).reduce((accumulator, currentValue) => {
      const status = nftsPinningStatus[currentValue]
      if (status?.code === BraveWallet.TokenPinStatusCode.STATUS_PINNED) {
        return accumulator += 1
      }

      return accumulator
    }, 0)
  }, [nftsPinningStatus])

  const status: BraveWallet.TokenPinStatusCode = React.useMemo(() => {
    if (pinnableNftsCount === pinnedNftsCount) {
      return BraveWallet.TokenPinStatusCode.STATUS_PINNED
    }

    const isUploading = Object.values(nftsPinningStatus).some(status => status?.code === BraveWallet.TokenPinStatusCode.STATUS_PINNING_IN_PROGRESS)
    if (isUploading) return BraveWallet.TokenPinStatusCode.STATUS_PINNING_IN_PROGRESS

    return BraveWallet.TokenPinStatusCode.STATUS_NOT_PINNED
  }, [pinnableNftsCount, nftsPinningStatus, pinnableNfts, pinnedNftsCount])

  const bannerStatus: BannerStatus = React.useMemo(() => {
    switch (status) {
      case BraveWallet.TokenPinStatusCode.STATUS_PINNED:
        return 'success'
      case BraveWallet.TokenPinStatusCode.STATUS_PINNING_IN_PROGRESS:
        return 'uploading'
      default:
        return 'start'
    }
  }, [status])

  const onLearnMore = React.useCallback(() => {
    history.push(WalletRoutes.LocalIpfsNode)
  }, [])

  return (
    <StyledWrapper status={bannerStatus}>
      <Row gap='12px' justifyContent='flex-start'>
        <NftPinningStatusAnimation
          size='30px'
          displayMode='banner'
          status={status}
        />
        <Text status={bannerStatus}>
          {bannerStatus === 'start' ? (
            <>
              Now you can run your IPFS and be part of web 3. Your NFT data will
              stay online forever and cannot be tampered with.&nbsp;
              <LearnMore onClick={onLearnMore}>Learn more</LearnMore>
            </>
          ) : bannerStatus === 'success' ? (
            `${pinnedNftsCount} out of ${pinnableNftsCount} NFTs have been successfully pinned to IPFS.`
          ) : (
            'You’re running IPFS node. File is being uploaded to IPFS.'
          )}
        </Text>
      </Row>
      {(bannerStatus === 'start' || bannerStatus === 'success') && (
        <CloseButton onClick={onDismiss} status={bannerStatus} />
      )}
    </StyledWrapper>
  )
}
