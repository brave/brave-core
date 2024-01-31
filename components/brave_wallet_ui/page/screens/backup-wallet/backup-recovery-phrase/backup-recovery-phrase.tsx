// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation } from 'react-router'
import { useDispatch } from 'react-redux'

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

// styles
import {
  ToggleVisibilityButton,
  LinkText,
  CopyButton,
  HorizontalSpace
} from '../../../../components/shared/style'
import {
  Description,
  NextButtonRow,
  Title,
  TitleAndDescriptionContainer,
  PhraseCard,
  PhraseCardBody,
  PhraseCardBottomRow,
  PhraseCardTopRow
} from '../../onboarding/onboarding.style'

// components
import { RecoveryPhrase } from '../../../../components/desktop/recovery-phrase/recovery-phrase'
import { CopiedToClipboardConfirmation } from '../../../../components/desktop/copied-to-clipboard-confirmation/copied-to-clipboard-confirmation'
import {
  NavButton //
} from '../../../../components/extension/buttons/nav-button/index'
import { OnboardingContentLayout } from '../../onboarding/components/onboarding-content-layout/onboarding-content-layout'

export const BackupRecoveryPhrase = () => {
  // routing
  const history = useHistory()
  const { pathname } = useLocation()
  const isOnboarding = pathname.includes(WalletRoutes.Onboarding)

  // redux
  const dispatch = useDispatch()
  const mnemonic = useSafePageSelector(PageSelectors.mnemonic)

  // mutations
  const [report] = useReportOnboardingActionMutation()

  // state
  const [isPhraseShown, setIsPhraseShown] = React.useState(false)

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
  console.log(skipBackup)

  const revealPhrase = React.useCallback(() => {
    setIsPhraseShown(true)
  }, [])

  const toggleShowPhrase = () => {
    setIsPhraseShown((prev) => !prev)
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
    <OnboardingContentLayout
      title={getLocale('braveWalletOnboardingRecoveryPhraseBackupIntroTitle')}
      subTitle={
        'Your recovery phrase is the key to access your wallet in case you forget your password or lose your device.'
      }
    >
      <TitleAndDescriptionContainer>
        <Title>{getLocale('braveWalletRecoveryPhraseBackupTitle')}</Title>
        <Description>
          {getLocale('braveWalletRecoveryPhraseBackupWarning')}
          <LinkText
            href='https://brave.com/learn/wallet-recovery-phrase/#how-should-i-store-my-recovery-phrase'
            target='_blank'
            rel='noreferrer'
            referrerPolicy='no-referrer'
          >
            {getLocale('braveWalletLearnMore')}
          </LinkText>
        </Description>
      </TitleAndDescriptionContainer>

      <PhraseCard>
        <PhraseCardTopRow>
          <ToggleVisibilityButton
            isVisible={isPhraseShown}
            onClick={toggleShowPhrase}
          />
        </PhraseCardTopRow>

        <PhraseCardBody>
          <RecoveryPhrase
            hidden={!isPhraseShown}
            onClickReveal={revealPhrase}
            recoveryPhrase={recoveryPhrase}
          />
        </PhraseCardBody>

        <PhraseCardBottomRow>
          <CopyButton onClick={onCopyPhrase} />

          {isCopied && (
            <>
              <CopiedToClipboardConfirmation />
              <HorizontalSpace space='52px' />
            </>
          )}
        </PhraseCardBottomRow>
      </PhraseCard>

      <NextButtonRow>
        <NavButton
          buttonType='primary'
          text={getLocale('braveWalletButtonNext')}
          url={
            isOnboarding
              ? WalletRoutes.OnboardingVerifyRecoveryPhrase
              : WalletRoutes.BackupVerifyRecoveryPhrase
          }
        />
      </NextButtonRow>
    </OnboardingContentLayout>
  )
}
