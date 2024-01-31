// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation } from 'react-router'
import { useDispatch } from 'react-redux'

// utils
import { getLocale, splitStringForTag } from '../../../../../common/locale'
import { WalletPageActions } from '../../../actions'
import {
  useReportOnboardingActionMutation //
} from '../../../../common/slices/api.slice'

// routes
import { BraveWallet, WalletRoutes } from '../../../../constants/types'
// import { WALLET_BACKUP_STEPS } from '../backup-wallet.routes'

// images
import ExamplePhrase from './images/example-recovery-phrase.svg'

// components
import {
  NavButton //
} from '../../../../components/extension/buttons/nav-button/index'

// style
import { NextButtonRow } from '../../onboarding/onboarding.style'
import {
  BannerCard,
  WarningCircle,
  ImportantText,
  BannerText,
  Subtitle,
  BackupInstructions
} from './explain-recovery-phrase.style'
import { VerticalSpace } from '../../../../components/shared/style'
import { OnboardingContentLayout } from '../../onboarding/components/onboarding-content-layout/onboarding-content-layout'

const importantTextParts = splitStringForTag(
  getLocale('braveWalletRecoveryPhraseBackupWarningImportant')
)

const ImportantTextSegments = () => {
  return (
    <BannerText>
      {importantTextParts.beforeTag}
      <ImportantText>{importantTextParts.duringTag}</ImportantText>
      {importantTextParts.afterTag}
    </BannerText>
  )
}

export const RecoveryPhraseExplainer = () => {
  // redux
  const dispatch = useDispatch()

  // mutations
  const [report] = useReportOnboardingActionMutation()

  // routing
  const history = useHistory()
  const { pathname } = useLocation()
  const isOnboarding = pathname.includes(WalletRoutes.Onboarding)

  // methods
  const skipBackup = () => {
    dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic: '' }))
    if (isOnboarding) {
      report(BraveWallet.OnboardingAction.CompleteRecoverySkipped)
      history.push(WalletRoutes.OnboardingComplete)
      return
    }
    history.push(WalletRoutes.PortfolioAssets)
  }

  console.log(skipBackup)

  // effects
  React.useEffect(() => {
    report(BraveWallet.OnboardingAction.RecoverySetup)
  }, [report])

  // render
  return (
    <OnboardingContentLayout
      title={getLocale('braveWalletOnboardingRecoveryPhraseBackupIntroTitle')}
      subTitle=''
    >
      <Subtitle>
        {getLocale('braveWalletOnboardingRecoveryPhraseBackupIntroDescription')}
      </Subtitle>
      <VerticalSpace space='14px' />
      <BackupInstructions>
        Keep it in a secure place that is not accessible to others and avoid
        sharing it with anyone.
      </BackupInstructions>
      <VerticalSpace space='54px' />

      <img
        width='376px'
        height='118px'
        src={ExamplePhrase}
      />

      <BannerCard>
        <WarningCircle />
        <ImportantTextSegments />
      </BannerCard>

      <NextButtonRow>
        <NavButton
          buttonType='primary'
          text={getLocale('braveWalletButtonGotIt')}
          url={
            isOnboarding
              ? WalletRoutes.OnboardingBackupRecoveryPhrase
              : WalletRoutes.BackupRecoveryPhrase
          }
        />
      </NextButtonRow>
    </OnboardingContentLayout>
  )
}

export default RecoveryPhraseExplainer
