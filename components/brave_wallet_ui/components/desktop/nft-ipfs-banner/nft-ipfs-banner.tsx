// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// constants
import { BraveWallet, WalletRoutes } from '../../../constants/types'

// utils
import { useNftPin } from '../../../common/hooks/nft-pin'
import { getLocale } from '../../../../common/locale'

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

  const { pinnableNftsCount, pinnedNftsCount, pinningStatusSummary: status } = useNftPin()

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
              {getLocale('braveWalletNftPinningBannerStart')}&nbsp;
              <LearnMore onClick={onLearnMore}>
                {getLocale('braveWalletNftPinningBannerLearnMore')}
              </LearnMore>
            </>
          ) : bannerStatus === 'success' ? (
            `${getLocale('braveWalletNftPinningBannerSuccess')
              .replace('$1', `${pinnedNftsCount}`)
              .replace('$2', `${pinnableNftsCount}`)}`
          ) : (
            `${getLocale('braveWalletNftPinningBannerUploading')}`
          )}
        </Text>
      </Row>
      {(bannerStatus === 'start' || bannerStatus === 'success') && (
        <CloseButton onClick={onDismiss} status={bannerStatus} />
      )}
    </StyledWrapper>
  )
}
