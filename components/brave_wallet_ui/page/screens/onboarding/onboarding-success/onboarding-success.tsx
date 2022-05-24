// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

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
import { WalletPageLayout } from '../../../../components/desktop'
import { NavButton } from '../../../../components/extension'
import { ArticleLinkBubble, ArticleLinkBubbleProps } from './components/article-link-bubble/article-link-bubble'

// styles
import {
  Description,
  Title,
  OnboardingWrapper
} from '../onboarding.style'
import {
  ArticleLinksContainer,
  ButtonContainer,
  IntroContainer,
  IntroImg
} from './onboarding-success.style'

const ARTICLES: ArticleLinkBubbleProps[] = [
  {
    icon: 'wallet',
    iconBackgroundColor: 'purple400',
    text: getLocale('braveWalletArticleLinkHowToUseAWallet'),
    url: 'https://brave.com/learn/what-is-crypto-wallet/#how-to-use-a-crypto-wallet'
  },
  {
    icon: 'wallet-with-coins',
    iconBackgroundColor: 'yellow500',
    text: getLocale('braveWalletArticleLinkWhatsTheBestWallet'),
    url: 'https://brave.com/learn/what-is-best-crypto-wallet/'
  },
  {
    icon: 'smartphone-desktop',
    iconBackgroundColor: 'green600',
    text: getLocale('braveWalletArticleLinkWhatsACryptoBrowser'),
    url: 'https://brave.com/learn/what-are-crypto-browsers/'
  },
  {
    icon: 'crypto-wallets',
    iconBackgroundColor: 'magenta400',
    text: getLocale('braveWalletArticleLinkWalletsBasics'),
    url: 'https://brave.com/learn/what-is-crypto-wallet/'
  },
  {
    icon: 'key',
    iconBackgroundColor: 'red200',
    text: getLocale('braveWalletArticleLinkWhatsARecoveryPhrase'),
    url: 'https://brave.com/learn/wallet-recovery-phrase/'
  },
  {
    icon: 'ipfs',
    iconBackgroundColor: 'orange300',
    text: getLocale('braveWalletArticleLinkWhatAreDapps'),
    url: 'https://brave.com/learn/what-are-dapps/'
  },
  {
    icon: 'grid',
    iconBackgroundColor: 'blue300',
    text: getLocale('braveWalletArticleLinkWhatIsWeb3'),
    url: 'https://brave.com/learn/what-is-web3/'
  }
]

const ArticleLinks = ARTICLES.map((article) => (
  <ArticleLinkBubble
    key={article.text}
    {...article}
  />
))

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
          getLocale('braveWalletOnboardingSuccessTitle')
        }</Title>

        <Description textAlign='center'>{
          getLocale('braveWalletOnboardingSuccessDescription')
        }</Description>

        <IntroImg src={WalletAccessSvg} height={118} />

      </IntroContainer>

      <ArticleLinksContainer>
        {ArticleLinks}
      </ArticleLinksContainer>

      <ButtonContainer>
        <NavButton
          buttonType='primary'
          text={getLocale('braveWalletGoToPortfolioButton')}
          onSubmit={onComplete}
        />

      </ButtonContainer>

    </OnboardingWrapper>
  </WalletPageLayout>
}
