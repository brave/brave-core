// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import { useHistory } from 'react-router'

// utils
import { getLocale } from '../../../../../common/locale'

// types
import { PageState, WalletRoutes } from '../../../../constants/types'

// hooks
import { useTemporaryCopyToClipboard } from '../../../../common/hooks/use-temporary-copy-to-clipboard'

// styles
import {
  ToggleVisibilityButton,
  GreenCheckmark
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
import {
  CopiedToClipboardContainer,
  CopyButton,
  DownloadButton,
  LinkText
} from './onboarding-backup-recovery-phrase.style'

// components
import { WalletPageLayout } from '../../../../components/desktop'
import { NavButton } from '../../../../components/extension'
import { RecoveryPhrase } from '../components/recovery-phrase/recovery-phrase'
import { OnboardingStepsNavigation } from '../components/onboarding-steps-navigation/onboarding-steps-navigation'

// storybook compiler thinks `randomUUID` doesnt exist
const randomUUID = () => (
  window.crypto as Crypto & { randomUUID: () => string }
).randomUUID()

export const OnboardingRecoveryPhrase = () => {
  // routing
  const history = useHistory()

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

  const nextStep = React.useCallback(() => {
    history.push(WalletRoutes.OnboardingVerifyRecoveryPhrase)
  }, [])

  const goBack = React.useCallback(() => {
    history.push(WalletRoutes.OnboardingExplainRecoveryPhrase)
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

          <OnboardingStepsNavigation
            goBack={goBack}
            currentStep={WalletRoutes.OnboardingBackupRecoveryPhrase}
          />

          <TitleAndDescriptionContainer>
            <Title>{getLocale('braveWalletRecoveryPhraseBackupTitle')}</Title>
            <Description>
              {getLocale('braveWalletRecoveryPhraseBackupWarning')}
              <LinkText
                href='https://brave.com/learn/wallet-recovery-phrase/'
                target='_blank'
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

              <CopyButton
                onClick={onCopyPhrase}
              />

              <a
                href={phraseDownloadUri}
                download={`brave-wallet-private-key-${fileGUID}.txt`}
                onClick={onDownloadPhraseFile}
              >
                <DownloadButton />
              </a>

              {isCopied &&
                <CopiedToClipboardContainer>
                  <GreenCheckmark />
                  <p>{getLocale('braveWalletCopiedToClipboard')}</p>
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
