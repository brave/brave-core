// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

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
import Illustration from '../../../assets/svg-icons/nft-ipfs/nfts-illustration.svg'

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
import { IpfsNodeStatus } from '../views/portfolio/components/ipfs-node-status/ipfs-node-status'
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

  const onClickCheckNfts = React.useCallback(() => {
    history.push(WalletRoutes.InspectNfts)
  }, [])

  const onClickRunNode = React.useCallback(() => {
    dispatch(WalletPageActions.setAutoPinEnabled(true))
  }, [])

  const goToNftsTab = React.useCallback(() => {
    history.push(WalletRoutes.Nfts)
  }, [])

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
              {isAutoPinEnabled
                ? <>
                  <ActionButton onClick={goToNftsTab}>{getLocale('braveWalletNftPinningPinNftsButton')}</ActionButton>
                  <IpfsNodeStatus />
                </>
                : <ActionButton onClick={onClickRunNode}>{getLocale('braveWalletNftPinningRunNodeButton')}</ActionButton>
              }
            </Row>
            <CheckNftsButton onClick={onClickCheckNfts}>{getLocale('braveWalletNftPinningCheckNftsButton')}</CheckNftsButton>
          </LeftWrapper>
          <RightWrapper>
            <BenefitHeading>{getLocale('braveWalletNftPinningBenefitsHeading')}</BenefitHeading>
            <BenefitsList>
              <li>{getLocale('braveWalletNftPinningBenefitOne')}</li>
              <li>{getLocale('braveWalletNftPinningBenefitTwo')}</li>
              <li>{getLocale('braveWalletNftPinningBenefitThree')}</li>
            </BenefitsList>
          </RightWrapper>
        </Section>
      </MainContent>
      <NftIllustration src={Illustration} />
      {/* TODO (william): Remove this button, only for testing */}
      <button onClick={() => {
        dispatch(WalletPageActions.setAutoPinEnabled(false))
      }}>disable autopin</button>
    </RunNodeWrapper>
  )
}
