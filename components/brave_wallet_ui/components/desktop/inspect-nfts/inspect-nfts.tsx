// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// api
import {
  useGetAutopinEnabledQuery,
  useGetPinnableVisibleNftIdsQuery,
  useSetAutopinEnabledMutation
} from '../../../common/slices/api.slice'

// components
import { NftList } from './components/nft-list/nft-list'
import Illustration from '../../../assets/png-icons/nft-ipfs/pinned-nft-illustration.png'
import { InfoTooltip } from './components/info-tooltip/info-tooltip'

// styled components
import { Row } from '../../shared/style'

// utils
import { getLocale } from '../../../../common/locale'

// routes
import { WalletRoutes } from '../../../constants/types'

// styles
import {
  InspectNftsWrapper,
  TopRow,
  TopRowButton,
  CloseIcon,
  BackIcon,
  MainContent,
  PinNftsButton,
  InfoSubHeading,
  InfoIcon,
  SubDivider,
  PinnedNftIllustration
} from './inspects-nfts.styles'
import { Description } from '../local-ipfs-node/local-ipfs-node.styles'

interface Props {
  onClose: () => void
  onBack: () => void
}

export const InspectNftsScreen = ({ onClose }: Props) => {
  const [showTooltip, setShowTooltip] = React.useState<boolean>(false)

  // hooks
  const { data: pinnableNftIds = [] } = useGetPinnableVisibleNftIdsQuery()
  const pinnableNftsCount = pinnableNftIds.length

  // routing
  const history = useHistory()

  // queries
  const { data: isAutoPinEnabled } = useGetAutopinEnabledQuery()

  // mutations
  const [setAutoPinStatus] = useSetAutopinEnabledMutation()

  // methods
  const onClickRunNode = React.useCallback(() => {
    if (!isAutoPinEnabled) {
      setAutoPinStatus(true)
    }
    history.push(WalletRoutes.PortfolioNFTs)
  }, [history, isAutoPinEnabled, setAutoPinStatus])

  const onShowTooltip = React.useCallback(() => setShowTooltip(true), [])
  const onHideTooltip = React.useCallback(() => setShowTooltip(false), [])

  return (
    <InspectNftsWrapper>
      <TopRow>
        <TopRowButton onClick={onClose}>
          <BackIcon />
          {getLocale('braveWalletNftPinningBackButton')}
        </TopRowButton>
        <TopRowButton onClick={onClose}>
          {getLocale('braveWalletNftPinningCloseButton')}
          <CloseIcon />
        </TopRowButton>
      </TopRow>
      <MainContent>
        <Row
          maxWidth='100%'
          alignItems='center'
          justifyContent='center'
        >
          <NftList />
        </Row>
        <Row
          margin='35px 0 11px 0'
          gap='8px'
          alignItems='center'
          justifyContent='flex-end'
          onMouseEnter={onShowTooltip}
          onMouseLeave={onHideTooltip}
        >
          <InfoSubHeading>
            {getLocale('braveWalletNftPinningWhyNotAvailable')}
          </InfoSubHeading>
          <InfoIcon />
          {showTooltip && (
            <InfoTooltip text={getLocale('braveWalletNftPinningTooltip')} />
          )}
        </Row>
        <Row>
          <SubDivider />
        </Row>
        <Row margin='32px 0 0'>
          <Description>
            {getLocale('braveWalletNftPinningBenefitsHeading')}
          </Description>
        </Row>
        <Row
          gap='16px'
          alignItems='center'
          justifyContent='center'
        >
          <PinNftsButton
            onClick={onClickRunNode}
            disabled={pinnableNftsCount === 0}
          >
            {getLocale('braveWalletNftPinningPinNftsButton')}
          </PinNftsButton>
        </Row>
        <PinnedNftIllustration src={Illustration} />
      </MainContent>
    </InspectNftsWrapper>
  )
}
