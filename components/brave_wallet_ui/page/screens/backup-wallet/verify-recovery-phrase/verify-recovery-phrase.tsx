// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { useHistory, useLocation } from 'react-router'

// utils
import { getLocale, getLocaleWithTags } from '../../../../../common/locale'
import {
  formatOrdinals,
  getWordIndicesToVerfy,
  ORDINALS
} from '../../../../utils/ordinal-utils'

// routes
import { WalletRoutes } from '../../../../constants/types'
import { WALLET_BACKUP_STEPS } from '../backup-wallet.routes'

// actions
import { WalletPageActions } from '../../../actions'

// styles
import { ErrorText, ErrorXIcon } from '../../../../components/shared/style'
import {
  Description,
  MainWrapper,
  NextButtonRow,
  StyledWrapper,
  Title,
  TitleAndDescriptionContainer,
  PhraseCard,
  PhraseCardBody
} from '../../onboarding/onboarding.style'
import {
  ErrorTextRow
} from './verify-backup-recovery-phrase.style'

// components
import { CenteredPageLayout } from '../../../../components/desktop/centered-page-layout/centered-page-layout'
import { NavButton } from '../../../../components/extension/buttons/nav-button/index'
import { useSafePageSelector } from '../../../../common/hooks/use-safe-selector'
import { PageSelectors } from '../../../selectors'
import { OnboardingNewWalletStepsNavigation } from '../../onboarding/components/onboarding-steps-navigation/onboarding-steps-navigation'
import RecoveryPhrase from '../../../../components/desktop/recovery-phrase/recovery-phrase'
import { StepsNavigation } from '../../../../components/desktop/steps-navigation/steps-navigation'

export const VerifyRecoveryPhrase = () => {
  // state
  const [nextStepEnabled, setNextStepEnabled] = React.useState(false)
  const [hasSelectedWords, setHasSelectedWords] = React.useState(false)

  // redux
  const dispatch = useDispatch()
  const mnemonic = useSafePageSelector(PageSelectors.mnemonic)

  // routing
  const history = useHistory()
  const { pathname } = useLocation()
  const isOnboarding = pathname.includes(WalletRoutes.Onboarding)

  // methods
  const onSelectedWordsUpdated = React.useCallback((words: any[], doesWordOrderMatch: boolean) => {
    setHasSelectedWords(words.length === 3)
    setNextStepEnabled(doesWordOrderMatch)
  }, [])

  const onSkip = React.useCallback(() => {
    dispatch(WalletPageActions.walletSetupComplete(true))
    history.push(WalletRoutes.OnboardingComplete)
  }, [])

  const onSkipBackup = React.useCallback(() => {
    history.push(WalletRoutes.Portfolio)
  }, [])

  const onNextStep = React.useCallback(() => {
    if (isOnboarding) {
      dispatch(WalletPageActions.walletSetupComplete(true))
    }
    dispatch(WalletPageActions.walletBackupComplete())
    history.push(isOnboarding
      ? WalletRoutes.OnboardingComplete
      : WalletRoutes.Portfolio
    )
  }, [isOnboarding])

  // memos
  const recoveryPhrase = React.useMemo(() => {
    return (mnemonic || '').split(' ')
  }, [mnemonic])

  // memos
  const verificationIndices = React.useMemo(
    () => getWordIndicesToVerfy(recoveryPhrase.length),
    [recoveryPhrase.length]
  )

  // render
  return (
    <CenteredPageLayout>
      <MainWrapper>
        <StyledWrapper>

          {isOnboarding &&
            <OnboardingNewWalletStepsNavigation
              goBackUrl={WalletRoutes.OnboardingExplainRecoveryPhrase}
              currentStep={WalletRoutes.OnboardingVerifyRecoveryPhrase}
              onSkip={onSkip}
            />
          }
          {!isOnboarding &&
            <StepsNavigation
              steps={WALLET_BACKUP_STEPS}
              goBackUrl={WalletRoutes.BackupRecoveryPhrase}
              currentStep={WalletRoutes.BackupVerifyRecoveryPhrase}
              onSkip={onSkipBackup}
            />
          }

          <TitleAndDescriptionContainer>
            <Title>{getLocale('braveWalletVerifyRecoveryPhraseTitle')}</Title>
            <Description>
              <span>
                {getLocaleWithTags('braveWalletVerifyRecoveryPhraseInstructions', 3).map((text, i) => {
                  return <span key={text.duringTag || i}>
                    {text.beforeTag}
                    <strong>
                      {
                        text.duringTag
                          ?.replace('$7', ORDINALS[verificationIndices[0]])
                          ?.replace('$8', formatOrdinals(verificationIndices[0] + 1))
                          ?.replace('$9', ORDINALS[verificationIndices[1]])
                          ?.replace('$10', formatOrdinals(verificationIndices[1] + 1))
                          ?.replace('$11', ORDINALS[verificationIndices[2]])
                          ?.replace('$12', formatOrdinals(verificationIndices[2] + 1))
                      }
                    </strong>
                    {text.afterTag}
                  </span>
                })}
              </span>
            </Description>
          </TitleAndDescriptionContainer>

          <PhraseCard>
            <PhraseCardBody>
              <RecoveryPhrase
                verificationIndices={verificationIndices}
                verificationModeEnabled={true}
                hidden={false}
                recoveryPhrase={recoveryPhrase}
                onSelectedWordListChange={onSelectedWordsUpdated}
              />
            </PhraseCardBody>
          </PhraseCard>

          <ErrorTextRow hasError={!nextStepEnabled && hasSelectedWords}>
            {!nextStepEnabled && hasSelectedWords &&
              <>
                <ErrorXIcon />
                <ErrorText>
                  {getLocale('braveWalletVerifyPhraseError')}
                </ErrorText>
              </>
            }
          </ErrorTextRow>

          <NextButtonRow>
            <NavButton
              buttonType='primary'
              text={getLocale('braveWalletButtonNext')}
              disabled={!nextStepEnabled}
              onSubmit={onNextStep}
            />
          </NextButtonRow>

        </StyledWrapper>
      </MainWrapper>
    </CenteredPageLayout>
  )
}

export default VerifyRecoveryPhrase
