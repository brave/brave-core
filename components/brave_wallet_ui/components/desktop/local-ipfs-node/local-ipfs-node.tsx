// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { useHistory } from 'react-router'

// types
import { WalletRoutes } from '../../../constants/types'

// actions
import { WalletPageActions } from '../../../page/actions'

// selectors
import { useSafePageSelector } from '../../../common/hooks/use-safe-selector'
import { PageSelectors } from '../../../page/selectors'

// components
import { Row } from '../../shared/style'
import Illustration from '../../../assets/png-icons/nft-ipfs/nfts-illustration.png'

// styles
import {
  CloseButton,
  CloseIcon,
  HeadingWrapper,
  Heading,
  RunNodeWrapper,
  TopRow,
  MainContent,
  IpfsIcon,
  Section,
  SectionText,
  Description,
  ActionButton,
  CheckNftsButton,
  BenefitHeading,
  LeftWrapper,
  RightWrapper,
  BenefitsList,
  NftIllustration
} from './local-ipfs-node.styles'
import { getLocale } from '../../../../common/locale'

interface Props {
  onClose: () => void
}

export const LocalIpfsNodeScreen = (props: Props) => {
  const { onClose } = props

  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()
  const isAutoPinEnabled = useSafePageSelector(PageSelectors.isAutoPinEnabled)

  // methods
  const onClickCheckNfts = React.useCallback(() => {
    history.push(WalletRoutes.InspectNfts)
  }, [])

  const onClickRunNode = React.useCallback(() => {
    if (!isAutoPinEnabled) {
      dispatch(WalletPageActions.setAutoPinEnabled(true))
    }
    history.push(WalletRoutes.Nfts)
  }, [isAutoPinEnabled])

  return (
    <RunNodeWrapper>
      <TopRow>
        <CloseButton onClick={onClose}>
          {getLocale('braveWalletNftPinningCloseButton')}
          <CloseIcon />
        </CloseButton>
      </TopRow>
      <MainContent>
        <HeadingWrapper>
          <IpfsIcon />
          <Heading>{getLocale('braveWalletNftPinningHeading')}</Heading>
        </HeadingWrapper>
        <Section>
          <LeftWrapper>
            <SectionText>{getLocale('braveWalletNftPinningRunNodeHeading')}</SectionText>
          </LeftWrapper>
          <RightWrapper>
            <Description>
              {getLocale('braveWalletNftPinningRunNodeDescription')}
            </Description>
          </RightWrapper>
        </Section>
        <Section>
          <LeftWrapper>
            <Row gap='16px' alignItems='center' justifyContent='flex-start' margin='0 0 0 8px'>
              <ActionButton onClick={onClickRunNode}>{getLocale('braveWalletNftPinningPinNftsButton')}</ActionButton>
            </Row>
            <CheckNftsButton onClick={onClickCheckNfts}>{getLocale('braveWalletNftPinningCheckNftsButton')}</CheckNftsButton>
          </LeftWrapper>
          <RightWrapper>
            <BenefitHeading>{getLocale('braveWalletNftPinningBenefitsHeading')}</BenefitHeading>
            <BenefitsList>
              <li>{getLocale('braveWalletNftPinningBenefitOne')}</li>
              <li>{getLocale('braveWalletNftPinningBenefitTwo')}</li>
            </BenefitsList>
          </RightWrapper>
        </Section>
      </MainContent>
      <NftIllustration src={Illustration} />
    </RunNodeWrapper>
  )
}
