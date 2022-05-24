// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { useDispatch } from 'react-redux'

// utils
// import { getLocale } from '../../../../../common/locale'

// images
import WalletAccessSvg from './images/wallet-access.svg'

// routes
import { WalletRoutes } from '../../../../constants/types'

// actions
import { WalletPageActions } from '../../../actions'

// components
import { WalletPageLayout } from '../../../../components/desktop'
import { NavButton } from '../../../../components/extension'
import { ArticleLinkBubble } from './components/article-link-bubble/article-link-bubble'

// styles
import {
  Description,
  Title,
  OnboardingWrapper
} from '../onboarding.style'
import {
  ButtonContainer,
  IntroContainer,
  IntroImg
} from './onboarding-success.style'

export const OnboardingSuccess = () => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()

  // methods
  const onComplete = React.useCallback(() => {
    dispatch(WalletPageActions.walletSetupComplete())
    history.push(WalletRoutes.Portfolio)
  }, [])

  // render
  return <WalletPageLayout>
    <OnboardingWrapper>

      <IntroContainer>
        <Title>{
          // getLocale('braveWalletWelcomeTitle')
          'Congrats! Your wallet is ready'
        }</Title>

        <Description textAlign='center'>{
          // getLocale('braveWalletWelcomeDescription')
          'Now you can easily access your wallet any time from the wallet icon in Brave Browser.'
        }</Description>

        <IntroImg src={WalletAccessSvg} height={118} />

      </IntroContainer>

      <ArticleLinkBubble
        icon='key'
        iconBackgroundColor='red200'
        text='What’s a recovery phrase?'
        url='brave.com'
      />

      <ButtonContainer>
        <NavButton
          buttonType='primary'
          text={
            'Go To Portfolio' // TODO: locale
          }
          onSubmit={onComplete}
        />

      </ButtonContainer>

    </OnboardingWrapper>
  </WalletPageLayout>
}
