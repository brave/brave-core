// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation } from 'react-router'
import { useDispatch } from 'react-redux'
import Button from '@brave/leo/react/button'
import * as leo from '@brave/leo/tokens/css'

// utils
import { getLocale } from '../../../../../common/locale'
import { PageSelectors } from '../../../selectors'
import { WalletPageActions } from '../../../actions'
import {
  useReportOnboardingActionMutation //
} from '../../../../common/slices/api.slice'

// routes
import { BraveWallet, WalletRoutes } from '../../../../constants/types'
// import { WALLET_BACKUP_STEPS } from '../backup-wallet.routes'

// hooks
import { useTemporaryCopyToClipboard } from '../../../../common/hooks/use-copy-to-clipboard'
import { useSafePageSelector } from '../../../../common/hooks/use-safe-selector'

// components
import { RecoveryPhrase } from '../../../../components/desktop/recovery-phrase/recovery-phrase'
import { OnboardingContentLayout } from '../../onboarding/components/onboarding-content-layout/onboarding-content-layout'
import {
  BackupInstructions,
  Subtitle,
  CopyButton,
  CopiedTick,
  CopyText
} from '../explain-recovery-phrase/explain-recovery-phrase.style'
import { SkipWarningDialog } from '../explain-recovery-phrase/skip-warning-dialog'

// styles
import { VerticalSpace } from '../../../../components/shared/style'
import {
  ContinueButton,
  NextButtonRow,
  PhraseCard,
  PhraseCardBody,
  PhraseCardBottomRow
} from '../../onboarding/onboarding.style'

export const BackupRecoveryPhrase = () => {
  // state
  const [isSkipWarningOpen, setIsSkipWarningOpen] = React.useState(false)

  // routing
  const history = useHistory()
  const { pathname } = useLocation()
  const isOnboarding = pathname.includes(WalletRoutes.Onboarding)

  // redux
  const dispatch = useDispatch()
  const mnemonic = useSafePageSelector(PageSelectors.mnemonic)

  // mutations
  const [report] = useReportOnboardingActionMutation()

  // custom hooks
  const { isCopied, temporaryCopyToClipboard } = useTemporaryCopyToClipboard()

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

  const onCopyPhrase = async () => {
    await temporaryCopyToClipboard(mnemonic || '')
  }

  // memos
  const recoveryPhrase = React.useMemo(() => {
    return (mnemonic || '').split(' ')
  }, [mnemonic])

  // render
  return (
    <>
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
        <VerticalSpace space='54px' />

        <PhraseCard>
          <PhraseCardBody>
            <RecoveryPhrase
              recoveryPhrase={recoveryPhrase}
            />
          </PhraseCardBody>

          <VerticalSpace space='24px' />

          <PhraseCardBottomRow>
            <CopyButton
              onClick={() => onCopyPhrase()}
              isCopied={isCopied}
            >
              {isCopied ? (
                <>
                  <CopyText>
                    {getLocale('braveWalletToolTipCopiedToClipboard')}
                  </CopyText>
                  <div slot='icon-after'>
                    <CopiedTick />
                  </div>
                </>
              ) : (
                <CopyText>Click to copy</CopyText>
              )}
            </CopyButton>
          </PhraseCardBottomRow>
        </PhraseCard>

        <VerticalSpace space='24px' />

        <NextButtonRow>
          <ContinueButton
            onClick={() =>
              history.push(
                isOnboarding
                  ? WalletRoutes.OnboardingVerifyRecoveryPhrase
                  : WalletRoutes.BackupVerifyRecoveryPhrase
              )
            }
          >
            {getLocale('braveWalletButtonContinue')}
          </ContinueButton>
        </NextButtonRow>
        <Button
          kind='plain-faint'
          color={leo.color.text.secondary}
          onClick={() => setIsSkipWarningOpen(true)}
        >
          {getLocale('braveWalletButtonSkip')}
        </Button>
      </OnboardingContentLayout>
      <SkipWarningDialog
        isOpen={isSkipWarningOpen}
        onBack={() => setIsSkipWarningOpen(false)}
        onSkip={skipBackup}
      />
    </>
  )
}
