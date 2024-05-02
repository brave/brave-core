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

// hooks
import {
  useTemporaryCopyToClipboard //
} from '../../../../common/hooks/use-copy-to-clipboard'
import { useSafePageSelector } from '../../../../common/hooks/use-safe-selector'

// components
import {
  RecoveryPhrase //
} from '../../../../components/desktop/recovery-phrase/recovery-phrase'
import {
  OnboardingContentLayout //
} from '../../onboarding/components/onboarding_content_layout/content_layout'
import {
  SkipWarningDialog //
} from '../explain-recovery-phrase/skip_warning_dialog'

// styles
import {
  BackupInstructions,
  Subtitle,
  CopyButton,
  CopiedTick,
  CopyText
} from '../explain-recovery-phrase/explain-recovery-phrase.style'
import {
  ContinueButton,
  NextButtonRow,
  PhraseCard,
  PhraseCardBody,
  PhraseCardBottomRow
} from '../../onboarding/onboarding.style'
import { ScrollableColumn } from '../../../../components/shared/style'

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
    <ScrollableColumn>
      <OnboardingContentLayout
        title={getLocale('braveWalletOnboardingRecoveryPhraseBackupIntroTitle')}
        subTitle=''
      >
        <Subtitle>
          {getLocale(
            'braveWalletOnboardingRecoveryPhraseBackupIntroDescription'
          )}
        </Subtitle>
        <BackupInstructions>
          {getLocale('braveWalletRecoveryPhraseBackupWarningImportant')}
        </BackupInstructions>

        <PhraseCard>
          <PhraseCardBody>
            <RecoveryPhrase recoveryPhrase={recoveryPhrase} />
          </PhraseCardBody>

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
                <CopyText>{getLocale('braveWalletClickToCopy')}</CopyText>
              )}
            </CopyButton>
          </PhraseCardBottomRow>
        </PhraseCard>

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
    </ScrollableColumn>
  )
}
