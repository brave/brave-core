// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// utils
import { getLocale } from '../../../../../common/locale'

// types
import { PageState, WalletRoutes } from '../../../../constants/types'

// hooks
import { useTemporaryCopyToClipboard } from '../../../../common/hooks/use-copy-to-clipboard'

// styles
import {
  ToggleVisibilityButton,
  LinkText,
  CopyButton,
  HorizontalSpace
} from '../../../../components/shared/style'
import {
  Description,
  MainWrapper,
  NextButtonRow,
  StyledWrapper,
  Title,
  TitleAndDescriptionContainer,
  PhraseCard,
  PhraseCardBody,
  PhraseCardBottomRow,
  PhraseCardTopRow
} from '../onboarding.style'

// components
import { RecoveryPhrase } from '../components/recovery-phrase/recovery-phrase'
import { CenteredPageLayout } from '../../../../components/desktop/centered-page-layout/centered-page-layout'
import { CopiedToClipboardConfirmation } from '../../../../components/desktop/copied-to-clipboard-confirmation/copied-to-clipboard-confirmation'
import { OnboardingNewWalletStepsNavigation } from '../components/onboarding-steps-navigation/onboarding-steps-navigation'
import { NavButton } from '../../../../components/extension'

export const OnboardingBackupRecoveryPhrase = () => {
  // redux
  const mnemonic = useSelector(({ page }: { page: PageState }) => page.mnemonic)

  // state
  const [isPhraseShown, setIsPhraseShown] = React.useState(false)

  // custom hooks
  const {
    isCopied,
    temporaryCopyToClipboard
  } = useTemporaryCopyToClipboard()

  // methods
  const revealPhrase = React.useCallback(() => {
    setIsPhraseShown(true)
  }, [])

  const toggleShowPhrase = () => {
    setIsPhraseShown(prev => !prev)
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
    <CenteredPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <OnboardingNewWalletStepsNavigation
            goBackUrl={WalletRoutes.OnboardingExplainRecoveryPhrase}
            currentStep={WalletRoutes.OnboardingBackupRecoveryPhrase}
          />

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

              {isCopied &&
                <>
                  <CopiedToClipboardConfirmation />
                  <HorizontalSpace space='52px' />
                </>
              }

            </PhraseCardBottomRow>
          </PhraseCard>

          <NextButtonRow>
            <NavButton
              buttonType='primary'
              text={getLocale('braveWalletButtonNext')}
              url={WalletRoutes.OnboardingVerifyRecoveryPhrase}
            />
          </NextButtonRow>

        </StyledWrapper>
      </MainWrapper>
    </CenteredPageLayout>
  )
}
