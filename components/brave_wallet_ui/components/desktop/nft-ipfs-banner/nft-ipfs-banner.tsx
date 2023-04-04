// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// constants
import { WalletRoutes } from '../../../constants/types'

// utils
import { OverallPinningStatus, useNftPin } from '../../../common/hooks/nft-pin'
import { getLocale } from '../../../../common/locale'

// selectors
import { useSafePageSelector } from '../../../common/hooks/use-safe-selector'
import { PageSelectors } from '../../../page/selectors'

// components
import { Row } from '../../shared/style'
import { NftPinningStatusAnimation } from '../nft-pinning-status-animation/nft-pinning-status-animation'

export type BannerStatus = 'start' | 'uploading' | 'success' | 'hidden'

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

  // redux
  const { pinnableNftsCount, pinnedNftsCount, pinningStatusSummary: status } = useNftPin()
  const isAutoPinEnabled = useSafePageSelector(PageSelectors.isAutoPinEnabled)

  // memos
  const bannerStatus: BannerStatus = React.useMemo(() => {
    if (!isAutoPinEnabled) return 'start'

    switch (status) {
      case OverallPinningStatus.PINNING_FINISHED:
        return 'success'
      case OverallPinningStatus.PINNING_IN_PROGRESS:
        return 'uploading'
      default:
        return 'hidden'
    }
  }, [status, pinnableNftsCount, isAutoPinEnabled])

  // methods
  const onLearnMore = React.useCallback(() => {
    history.push(WalletRoutes.LocalIpfsNode)
  }, [])

  return (
    <StyledWrapper status={bannerStatus}>
      <Row gap='12px' justifyContent='flex-start'>
        <NftPinningStatusAnimation
          size='14px'
          status={status}
          isAutopinEnabled={isAutoPinEnabled}
        />
        <Text status={bannerStatus}>
          {bannerStatus === 'start' ? (
            <>
              {getLocale('braveWalletNftPinningBannerStart')}
            </>
          ) : bannerStatus === 'success' ? (
            `${getLocale('braveWalletNftPinningBannerSuccess')
              .replace('$1', `${pinnedNftsCount}`)}`
          ) : (
            `${getLocale('braveWalletNftPinningBannerUploading')}`
          )}
        </Text>
        {bannerStatus === 'start' &&
          <LearnMore onClick={onLearnMore}>
            {getLocale('braveWalletNftPinningBannerLearnMore')}
          </LearnMore>
        }
      </Row>
      {(bannerStatus === 'success') && (
        <CloseButton onClick={onDismiss} />
      )}
    </StyledWrapper>
  )
}
