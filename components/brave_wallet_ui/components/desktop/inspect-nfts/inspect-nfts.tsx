// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { useHistory } from 'react-router'

// components
import { NftList } from './components/nft-list/nft-list'
import Illustration from '../../../assets/svg-icons/nft-ipfs/pinned-nft-illustration.svg'
import { InfoTooltip } from './components/info-tooltip/info-tooltip'
import { IpfsNodeStatus } from '../views/portfolio/components/ipfs-node-status/ipfs-node-status'
import {
  ActionButton,
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

  const onClickRunNode = React.useCallback(() => {
    dispatch(WalletPageActions.setAutoPinEnabled(true))
  }, [])

  const onShowTooltip = React.useCallback(() => setShowTooltip(true), [])
  const onHideTooltip = React.useCallback(() => setShowTooltip(false), [])

  const goToNftsTab = React.useCallback(() => {
    history.push(WalletRoutes.Nfts)
  }, [])

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
            <li>{getLocale('braveWalletNftPinningBenefitThree')}</li>
          </BenefitsList>
        </Column>
        <Row gap='16px' alignItems='center' justifyContent='flex-start'>
          {isAutoPinEnabled
            ? <>
              <PinNftsButton onClick={goToNftsTab}>{getLocale('braveWalletNftPinningPinNftsButton')}</PinNftsButton>
              <IpfsNodeStatus />
            </>
            : <ActionButton onClick={onClickRunNode}>{getLocale('braveWalletNftPinningRunNodeButton')}</ActionButton>
          }
        </Row>
        <PinnedNftIllustration src={Illustration} />
      </MainContent>
    </InspectNftsWrapper>
  )
}
