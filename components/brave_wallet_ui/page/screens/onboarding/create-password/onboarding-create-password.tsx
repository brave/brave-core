// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { useDispatch } from 'react-redux'

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
import { OnboardingNewWalletStepsNavigation } from '../components/onboarding-steps-navigation/onboarding-steps-navigation'

// styles
import {
  StyledWrapper,
  Title,
  Description,
  NextButtonRow,
  MainWrapper
} from '../onboarding.style'

export const OnboardingCreatePassword = () => {
  // routing
  const history = useHistory()

  // redux
  const dispatch = useDispatch()

  // state
  const [isValid, setIsValid] = React.useState(false)
  const [password, setPassword] = React.useState('')

  // methods
  const nextStep = React.useCallback(() => {
    if (isValid) {
      dispatch(WalletPageActions.createWallet({ password }))
      history.push(WalletRoutes.OnboardingExplainRecoveryPhrase)
    }
  }, [password, isValid])

  const handlePasswordChange = React.useCallback(({ isValid, password }: NewPasswordValues) => {
    setPassword(password)
    setIsValid(isValid)
  }, [])

  // render
  return (
    <WalletPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <OnboardingNewWalletStepsNavigation
            goBackUrl={WalletRoutes.OnboardingWelcome}
            currentStep={WalletRoutes.OnboardingCreatePassword}
            preventSkipAhead
          />

          <div>
            <Title>{getLocale('braveWalletCreatePasswordTitle')}</Title>
            <Description>{getLocale('braveWalletCreatePasswordDescription')}</Description>
          </div>

          <NewPasswordInput
            autoFocus={true}
            onSubmit={nextStep}
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
