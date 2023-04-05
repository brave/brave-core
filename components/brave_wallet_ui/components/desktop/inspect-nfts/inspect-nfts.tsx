// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { useHistory } from 'react-router'

// components
import { NftList } from './components/nft-list/nft-list'
import Illustration from '../../../assets/png-icons/nft-ipfs/pinned-nft-illustration.png'
import { InfoTooltip } from './components/info-tooltip/info-tooltip'

// styled components
import {
  BenefitHeading,
  BenefitsList
} from '../local-ipfs-node/local-ipfs-node.styles'
import { Column, Row } from '../../shared/style'

// selectors
import { useSafePageSelector } from '../../../common/hooks/use-safe-selector'
import { PageSelectors } from '../../../page/selectors'

// utils
import { WalletPageActions } from '../../../page/actions'
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

interface Props {
  onClose: () => void
  onBack: () => void
}

export const InspectNftsScreen = ({ onClose }: Props) => {
  const [showTooltip, setShowTooltip] = React.useState<boolean>(false)

  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()
  const isAutoPinEnabled = useSafePageSelector(PageSelectors.isAutoPinEnabled)

  // methods
  const onClickRunNode = React.useCallback(() => {
    if (!isAutoPinEnabled) {
      dispatch(WalletPageActions.setAutoPinEnabled(true))
    }
    history.push(WalletRoutes.Nfts)
  }, [isAutoPinEnabled])

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
        <Row maxWidth='100%' alignItems='center' justifyContent='center'>
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
          <InfoSubHeading>{getLocale('braveWalletNftPinningWhyNotAvailable')}</InfoSubHeading>
          <InfoIcon/>
          {showTooltip && (
            <InfoTooltip text={getLocale('braveWalletNftPinningTooltip')} />
          )}
        </Row>
        <Row>
          <SubDivider />
        </Row>
        <Column margin='32px 0 0'>
          <BenefitHeading>{getLocale('braveWalletNftPinningBenefitsHeading')}</BenefitHeading>
          <BenefitsList>
            <li>{getLocale('braveWalletNftPinningBenefitOne')}</li>
            <li>{getLocale('braveWalletNftPinningBenefitTwo')}</li>
          </BenefitsList>
        </Column>
        <Row gap='16px' alignItems='center' justifyContent='center'>
          <PinNftsButton onClick={onClickRunNode}>{getLocale('braveWalletNftPinningPinNftsButton')}</PinNftsButton>
        </Row>
        <PinnedNftIllustration src={Illustration} />
      </MainContent>
    </InspectNftsWrapper>
  )
}
