// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation } from 'react-router'

// redux
import { useAppDispatch } from '../../../../common/hooks/use-redux'

// utils
import { getLocale } from '../../../../../common/locale'
import { splitRecoveryPhraseWords } from '../../../../utils/recovery-phrase-utils'
import { PageSelectors } from '../../../selectors'
import { WalletPageActions } from '../../../actions'

// routes
import { WalletRoutes } from '../../../../constants/types'

// hooks
import {
  useTemporaryCopyToClipboard, //
} from '../../../../common/hooks/use-copy-to-clipboard'
import { useSafePageSelector } from '../../../../common/hooks/use-safe-selector'

// components
import {
  RecoveryPhrase, //
} from '../../../../components/desktop/recovery-phrase/recovery-phrase'
import {
  OnboardingContentLayout, //
} from '../../onboarding/components/onboarding_content_layout/content_layout'
import {
  SkipWarningDialog, //
} from '../explain-recovery-phrase/skip_warning_dialog'

// styles
import {
  BackupInstructions,
  Subtitle,
  CopyButton,
  CopiedTick,
  CopyText,
} from '../explain-recovery-phrase/explain-recovery-phrase.style'
import {
  ContinueButton,
  SkipButton,
  NextButtonRow,
  PhraseCard,
  PhraseCardBody,
  PhraseCardBottomRow,
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
  const dispatch = useAppDispatch()
  const mnemonic = useSafePageSelector(PageSelectors.mnemonic)

  // custom hooks
  const { isCopied, temporaryCopyToClipboard } = useTemporaryCopyToClipboard(
    undefined,
    true,
  )

  // methods
  const skipBackup = () => {
    dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic: '' }))
    if (isOnboarding) {
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
    return splitRecoveryPhraseWords(mnemonic || '')
  }, [mnemonic])

  // render
  return (
    <ScrollableColumn>
      <OnboardingContentLayout
        title={getLocale(
          S.BRAVE_WALLET_ONBOARDING_RECOVERY_PHRASE_BACKUP_INTRO_TITLE,
        )}
        subTitle=''
      >
        <Subtitle>
          {getLocale(
            S.BRAVE_WALLET_ONBOARDING_RECOVERY_PHRASE_BACKUP_INTRO_DESCRIPTION,
          )}
        </Subtitle>
        <BackupInstructions>
          {getLocale(S.BRAVE_WALLET_RECOVERY_PHRASE_BACKUP_WARNING_IMPORTANT)}
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
                    {getLocale(S.BRAVE_WALLET_TOOL_TIP_COPIED_TO_CLIPBOARD)}
                  </CopyText>
                  <div slot='icon-after'>
                    <CopiedTick />
                  </div>
                </>
              ) : (
                <CopyText>{getLocale(S.BRAVE_WALLET_CLICK_TO_COPY)}</CopyText>
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
                  : WalletRoutes.BackupVerifyRecoveryPhrase,
              )
            }
          >
            {getLocale(S.BRAVE_WALLET_BUTTON_CONTINUE)}
          </ContinueButton>
        </NextButtonRow>
        <SkipButton
          kind='plain-faint'
          onClick={() => setIsSkipWarningOpen(true)}
        >
          {getLocale(S.BRAVE_WALLET_BUTTON_SKIP)}
        </SkipButton>
      </OnboardingContentLayout>
      <SkipWarningDialog
        isOpen={isSkipWarningOpen}
        onBack={() => setIsSkipWarningOpen(false)}
        onSkip={skipBackup}
      />
    </ScrollableColumn>
  )
}
