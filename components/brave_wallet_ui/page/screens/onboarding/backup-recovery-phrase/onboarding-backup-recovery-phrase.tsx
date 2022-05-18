// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import { useHistory } from 'react-router'

import {
  CopiedToClipboardContainer,
  CopyButton,
  DownloadButton,
  LinkText,
  PhraseCard,
  PhraseCardBody,
  PhraseCardBottomRow,
  PhraseCardTopRow
} from './onboarding-backup-recovery-phrase.style'

import {
  Description,
  MainWrapper,
  NextButtonRow,
  StyledWrapper,
  Title,
  TitleAndDescriptionContainer
} from '../onboarding.style'

// utils
import { getLocale } from '../../../../../common/locale'

// routes
import { PageState, WalletRoutes } from '../../../../constants/types'

// hooks
import { useTemporaryCopyToClipboard } from '../../../../common/hooks/use-temporary-copy-to-clipboard'

// components
import { WalletPageLayout } from '../../../../components/desktop'
import { GreenCheckmark } from '../../../../components/shared/tooltip/password-strength-tooltip.style'
import { NavButton } from '../../../../components/extension'
import { OnboardingSteps, OnboardingStepsNavigation } from '../components/onboarding-steps-navigation/onboarding-steps-navigation'
import { ToggleVisibilityButton } from '../../../../components/shared/style'
import { RecoveryPhrase } from './components/recovery-phrase'

export const OnboardingRecoveryPhrase = () => {
  // routing
  const history = useHistory()

  // redux
  const { mnemonic } = useSelector(({ page }: { page: PageState }) => page)

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

  const toggleShowPhrase = React.useCallback(() => {
    setIsPhraseShown(prev => !prev)
  }, [])

  const nextStep = React.useCallback(() => {
    history.push(WalletRoutes.OnboardingVerifyRecoveryPhrase)
  }, [])

  const goBack = React.useCallback(() => {
    history.push(WalletRoutes.OnboardingExplainRecoveryPhrase)
  }, [])

  const onCopyPhrase = React.useCallback(async () => {
    await temporaryCopyToClipboard(mnemonic || '')
  }, [mnemonic])

  // memos
  const recoveryPhrase = React.useMemo(() => {
    return (mnemonic || '').split(' ')
  }, [mnemonic])

  const phraseDownloadUri = React.useMemo(
    () => `data:text/plain;charset=utf-8,${
      encodeURI(recoveryPhrase.join(' '))
    }`,
    [recoveryPhrase]
  )

  // render
  return (
    <WalletPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <OnboardingStepsNavigation
            goBack={goBack}
            currentStep={OnboardingSteps.backupRecoveryPhrase}
          />

          <TitleAndDescriptionContainer>
            <Title>{getLocale('braveWalletRecoveryPhraseBackupTitle')}</Title>
            <Description>
              {getLocale('braveWalletRecoveryPhraseBackupWarning')}
              <LinkText>{getLocale('braveWalletWelcomePanelButton')}</LinkText>
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

              <CopyButton
                onClick={onCopyPhrase}
              />

              <a
                href={phraseDownloadUri}
                download='brave-wallet-backup.txt'
              >
                <DownloadButton />
              </a>

              {isCopied &&
                <CopiedToClipboardContainer>
                  <GreenCheckmark />
                  <p>Copied to clipboard</p>
                </CopiedToClipboardContainer>
              }

            </PhraseCardBottomRow>
          </PhraseCard>

          <NextButtonRow>
            <NavButton
              buttonType='primary'
              text={getLocale('braveWalletButtonNext')}
              onSubmit={nextStep}
            />
          </NextButtonRow>

        </StyledWrapper>
      </MainWrapper>
    </WalletPageLayout>
  )
}

export default OnboardingRecoveryPhrase
