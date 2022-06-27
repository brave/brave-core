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
  DownloadButton,
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
import { WalletPageLayout } from '../../../../components/desktop'
import { NavButton } from '../../../../components/extension'
import { CopiedToClipboardConfirmation } from '../../../../components/desktop/copied-to-clipboard-confirmation/copied-to-clipboard-confirmation'
import { RecoveryPhrase } from '../components/recovery-phrase/recovery-phrase'
import { OnboardingNewWalletStepsNavigation } from '../components/onboarding-steps-navigation/onboarding-steps-navigation'

// storybook compiler thinks `randomUUID` doesnt exist
const randomUUID = () => (
  window.crypto as Crypto & { randomUUID: () => string }
).randomUUID()

export const OnboardingBackupRecoveryPhrase = () => {
  // redux
  const { mnemonic } = useSelector(({ page }: { page: PageState }) => page)

  // state
  const [isPhraseShown, setIsPhraseShown] = React.useState(false)
  const [fileGUID, setFileGUID] = React.useState(randomUUID())

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

  const onCopyPhrase = React.useCallback(async () => {
    await temporaryCopyToClipboard(mnemonic || '')
  }, [mnemonic])

  const onDownloadPhraseFile = React.useCallback(() => {
    setFileGUID(randomUUID())
  }, [])

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
                {getLocale('braveWalletWelcomePanelButton')}
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

              <a
                href={phraseDownloadUri}
                download={`brave-wallet-private-key-${fileGUID}.txt`}
                onClick={onDownloadPhraseFile}
              >
                <DownloadButton />
              </a>

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
    </WalletPageLayout>
  )
}
