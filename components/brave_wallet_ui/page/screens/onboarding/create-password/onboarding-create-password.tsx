// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { useDispatch } from 'react-redux'

// styles
import {
  StyledWrapper,
  Title,
  Description,
  NextButtonRow,
  MainWrapper
} from '../onboarding.style'

// utils
import { getLocale } from '../../../../../common/locale'

// routes
import { WalletRoutes } from '../../../../constants/types'

// actions
import { WalletPageActions } from '../../../actions'

// components
import { WalletPageLayout } from '../../../../components/desktop'
import { NavButton } from '../../../../components/extension'
import { NewPasswordInput, NewPasswordValues } from '../../../../components/shared/password-input/new-password-input'
import { OnboardingSteps, OnboardingStepsNavigation } from '../components/onboarding-steps-navigation/onboarding-steps-navigation'

export const OnboardingCreatePassword = () => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()

  // state
  const [isValid, setIsValid] = React.useState(false)

  // methods
  const nextStep = React.useCallback(() => {
    history.push(WalletRoutes.OnboardingExplainRecoveryPhrase)
  }, [])

  const handlePasswordChange = React.useCallback(({ isValid, password }: NewPasswordValues) => {
    setIsValid(isValid)
  }, [])

  const handlePasswordSubmit = React.useCallback(({ isValid, password }: NewPasswordValues) => {
    alert(password)
    if (isValid) {
      dispatch(WalletPageActions.createWallet({ password }))
      nextStep()
    }
  }, [nextStep])

  const goBack = React.useCallback(() => {
    history.push(WalletRoutes.Onboarding)
  }, [])

  // render
  return (
    <WalletPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <OnboardingStepsNavigation
            goBack={goBack}
            currentStep={OnboardingSteps.createPassword}
            preventSkipAhead
          />

          <div>
            <Title>{getLocale('braveWalletCreatePasswordTitle')}</Title>
            <Description>{getLocale('braveWalletCreatePasswordDescription')}</Description>
          </div>

          <NewPasswordInput
            autoFocus={true}
            onSubmit={handlePasswordSubmit}
            onChange={handlePasswordChange}
          />

          <NextButtonRow>
            <NavButton
              buttonType='primary'
              text={getLocale('braveWalletButtonNext')}
              onSubmit={nextStep}
              disabled={!isValid}
            />
          </NextButtonRow>

        </StyledWrapper>
      </MainWrapper>
    </WalletPageLayout>
  )
}

export default OnboardingCreatePassword
