// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation } from 'react-router'
import { useDispatch } from 'react-redux'
import Button from '@brave/leo/react/button'
import * as leo from '@brave/leo/tokens/css/variables'

// utils
import { getLocale } from '../../../../../common/locale'
import { WalletPageActions } from '../../../actions'
import {
  useReportOnboardingActionMutation //
} from '../../../../common/slices/api.slice'

// routes
import { BraveWallet, WalletRoutes } from '../../../../constants/types'

// components
import { SkipWarningDialog } from './skip_warning_dialog'

// style
import { ContinueButton } from '../../onboarding/onboarding.style'
import {
  Subtitle,
  BackupInstructions,
  ExampleRecoveryPhrase
} from './explain-recovery-phrase.style'
import { Column, VerticalSpace } from '../../../../components/shared/style'
import {
  OnboardingContentLayout //
} from '../../onboarding/components/onboarding_content_layout/content_layout'

export const RecoveryPhraseExplainer = () => {
  // state
  const [isSkipWarningOpen, setIsSkipWarningOpen] = React.useState(false)

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

  const onContinue = () => {
    history.push(
      isOnboarding
        ? WalletRoutes.OnboardingBackupRecoveryPhrase
        : WalletRoutes.BackupRecoveryPhrase
    )
  }

  // effects
  React.useEffect(() => {
    report(BraveWallet.OnboardingAction.RecoverySetup)
  }, [report])

  // render
  return (
    <>
      <SkipWarningDialog
        isOpen={isSkipWarningOpen}
        onBack={() => setIsSkipWarningOpen(false)}
        onSkip={skipBackup}
      />
      <OnboardingContentLayout
        title={getLocale('braveWalletOnboardingRecoveryPhraseBackupIntroTitle')}
        subTitle=''
      >
        <Subtitle>
          {getLocale(
            'braveWalletOnboardingRecoveryPhraseBackupIntroDescription'
          )}
        </Subtitle>
        <VerticalSpace space='14px' />
        <BackupInstructions>
          {getLocale('braveWalletRecoveryPhraseBackupWarningImportant')}
        </BackupInstructions>
        <ExampleRecoveryPhrase />
        <Column gap='24px'>
          <ContinueButton onClick={onContinue}>
            {getLocale('braveWalletButtonVerifyPhrase')}
          </ContinueButton>
          <Button
            kind='plain-faint'
            color={leo.color.text.secondary}
            onClick={() => setIsSkipWarningOpen(true)}
          >
            {getLocale('braveWalletButtonSkip')}
          </Button>
        </Column>
      </OnboardingContentLayout>
    </>
  )
}

export default RecoveryPhraseExplainer
