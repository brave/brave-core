// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation } from 'react-router'

// utils
import { getLocale, splitStringForTag } from '../../../../../common/locale'

// routes
import { WalletRoutes } from '../../../../constants/types'
import { WALLET_BACKUP_STEPS } from '../backup-wallet.routes'

// images
import ExamplePhrase from './images/example-recovery-phrase.svg'

// components
import { NavButton } from '../../../../components/extension'
import { CenteredPageLayout } from '../../../../components/desktop/centered-page-layout/centered-page-layout'
import { OnboardingNewWalletStepsNavigation } from '../../onboarding/components/onboarding-steps-navigation/onboarding-steps-navigation'
import { ArticleLinkBubble } from '../../onboarding/onboarding-success/components/article-link-bubble/article-link-bubble'
import { StepsNavigation } from '../../../../components/desktop/steps-navigation/steps-navigation'

// style
import {
  StyledWrapper,
  Title,
  Description,
  NextButtonRow,
  MainWrapper
} from '../../onboarding/onboarding.style'
import {
  BannerCard,
  WarningCircle,
  ImportantText,
  BannerText,
  CenteredRow
} from './explain-recovery-phrase.style'

const importantTextParts = splitStringForTag(getLocale('braveWalletRecoveryPhraseBackupWarningImportant'))

const ImportantTextSegments = () => {
  return <BannerText>
    {importantTextParts.beforeTag}
    <ImportantText>{importantTextParts.duringTag}</ImportantText>
    {importantTextParts.afterTag}
  </BannerText>
}

export const RecoveryPhraseExplainer = () => {
  // routing
  const history = useHistory()
  const { pathname } = useLocation()
  const isOnboarding = pathname.includes(WalletRoutes.Onboarding)

  // methods
  const skipToOnboardingSuccess = () => {
    history.push(WalletRoutes.OnboardingComplete)
  }

  const skipBackup = () => {
    history.push(WalletRoutes.Portfolio)
  }

  // render
  return (
    <CenteredPageLayout>
      <MainWrapper>
        <StyledWrapper>

          {isOnboarding &&
            <OnboardingNewWalletStepsNavigation
              preventGoBack
              currentStep={WalletRoutes.OnboardingExplainRecoveryPhrase}
              onSkip={skipToOnboardingSuccess}
            />
          }
          {!isOnboarding &&
            <StepsNavigation
              steps={WALLET_BACKUP_STEPS}
              preventGoBack
              currentStep={WalletRoutes.BackupExplainRecoveryPhrase}
              preventSkipAhead
              onSkip={skipBackup}
            />
          }

          <div>
            <Title>{getLocale('braveWalletOnboardingRecoveryPhraseBackupIntroTitle')}</Title>
            <Description>{getLocale('braveWalletOnboardingRecoveryPhraseBackupIntroDescription')}</Description>
            <CenteredRow>
              <ArticleLinkBubble
                icon='key'
                iconBackgroundColor='red200'
                text={getLocale('braveWalletArticleLinkWhatsARecoveryPhrase')}
                url='https://brave.com/learn/wallet-recovery-phrase/'
              />
            </CenteredRow>
          </div>

          <img width='376px' height='118px' src={ExamplePhrase} />

          <BannerCard>
            <WarningCircle />
            <ImportantTextSegments />
          </BannerCard>

          <NextButtonRow>
            <NavButton
              buttonType='primary'
              text={getLocale('braveWalletButtonGotIt')}
              url={isOnboarding
                ? WalletRoutes.OnboardingBackupRecoveryPhrase
                : WalletRoutes.BackupRecoveryPhrase
              }
            />
          </NextButtonRow>

        </StyledWrapper>
      </MainWrapper>
    </CenteredPageLayout>
  )
}

export default RecoveryPhraseExplainer
