// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { useDispatch } from 'react-redux'

// utils
import { getLocale } from '../../../../../common/locale'

// images
import WalletAccessSvg from './images/wallet-access.svg'

// routes
import { WalletRoutes } from '../../../../constants/types'

// actions
import { WalletPageActions } from '../../../actions'

// components
import { NavButton } from '../../../../components/extension/buttons/nav-button/index'
import { CenteredPageLayout } from '../../../../components/desktop/centered-page-layout/centered-page-layout'

// styles
import { LinkText } from '../../../../components/shared/style'
import {
  Description,
  Title,
  StyledWrapper,
  MainWrapper
} from '../onboarding.style'
import {
  ArticleLinksContainer,
  ButtonContainer,
  IntroContainer,
  IntroImg,
  CloseButtonContainer,
  DepositIcon
} from './onboarding-success.style'

export const OnboardingSuccess = () => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()

  // methods
  const onComplete = React.useCallback(() => {
    dispatch(WalletPageActions.walletSetupComplete(true))
    history.push(WalletRoutes.Portfolio)
  }, [])

  const onClickBuyCrypto = React.useCallback(() => {
    dispatch(WalletPageActions.walletSetupComplete(true))
    history.push(WalletRoutes.FundWalletPageStart)
  }, [])

  const onClickDepositCrypto = React.useCallback(() => {
    dispatch(WalletPageActions.walletSetupComplete(true))
    history.push(WalletRoutes.DepositFundsPageStart)
  }, [])

  // render
  return <CenteredPageLayout>

    <MainWrapper>
      <StyledWrapper>
        <CloseButtonContainer>
          <LinkText onClick={onComplete}>
            {getLocale('braveWalletButtonDone')}
          </LinkText>
        </CloseButtonContainer>
      </StyledWrapper>

      <IntroContainer>
        <Title>{
          getLocale('braveWalletOnboardingSuccessTitle')
        }</Title>

        <Description>{
          getLocale('braveWalletOnboardingSuccessDescription')
        }</Description>

        <IntroImg src={WalletAccessSvg} height={138} />

      </IntroContainer>

      <ArticleLinksContainer>
        <LinkText
          rel="noreferrer"
          target='_blank'
          href='https://brave.com/learn/what-is-crypto-wallet/#how-to-use-a-crypto-wallet'
        >
          {getLocale('braveWalletLearnAboutMyWallet')}
        </LinkText>
      </ArticleLinksContainer>

      <ButtonContainer>
        <NavButton
          buttonType='primary'
          text={getLocale('braveWalletBuyCryptoButton')}
          onSubmit={onClickBuyCrypto}
        />

        <LinkText onClick={onClickDepositCrypto}>
          <DepositIcon />
          {getLocale('braveWalletDepositCryptoButton')}
        </LinkText>

      </ButtonContainer>

    </MainWrapper>
  </CenteredPageLayout>
}
