// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'

// utils
import { getLocale } from '../../../../../common/locale'
import { useApiProxy } from '../../../../common/hooks/use-api-proxy'

// routes
import { OnboardingAction, WalletRoutes, WalletState } from '../../../../constants/types'

// actions
import { WalletPageActions } from '../../../actions'

// components
import { NavButton } from '../../../../components/extension'
import { NewPasswordInput, NewPasswordValues } from '../../../../components/shared/password-input/new-password-input'
import { OnboardingNewWalletStepsNavigation } from '../components/onboarding-steps-navigation/onboarding-steps-navigation'
import { CenteredPageLayout } from '../../../../components/desktop/centered-page-layout/centered-page-layout'

// styles
import {
  StyledWrapper,
  Title,
  Description,
  NextButtonRow,
  MainWrapper,
  TitleAndDescriptionContainer
} from '../onboarding.style'

interface OnboardingCreatePasswordProps {
  isHardwareOnboarding?: boolean
  onWalletCreated: () => void
}

export const OnboardingCreatePassword = (props: OnboardingCreatePasswordProps) => {
  const { isHardwareOnboarding, onWalletCreated } = props

  // redux
  const dispatch = useDispatch()
  const isWalletCreated = useSelector(({ wallet }: { wallet: WalletState }) => wallet.isWalletCreated)

  // state
  const [isValid, setIsValid] = React.useState(false)
  const [password, setPassword] = React.useState('')

  // methods
  const nextStep = React.useCallback(() => {
    if (isValid) {
      dispatch(WalletPageActions.createWallet({ password }))
    }
  }, [password, isValid])

  const handlePasswordChange = React.useCallback(({ isValid, password }: NewPasswordValues) => {
    setPassword(password)
    setIsValid(isValid)
  }, [])

  // custom hooks
  const { braveWalletP3A } = useApiProxy()

  // effects
  React.useEffect(() => {
    if (isWalletCreated) {
      braveWalletP3A.reportOnboardingAction(OnboardingAction.CREATED_WALLET)
      onWalletCreated()
    }
  }, [isWalletCreated, onWalletCreated])

  // render
  return (
    <CenteredPageLayout>
      <MainWrapper>
        <StyledWrapper>

          <OnboardingNewWalletStepsNavigation
            goBackUrl={WalletRoutes.OnboardingWelcome}
            currentStep={WalletRoutes.OnboardingCreatePassword}
            isHardwareOnboarding={isHardwareOnboarding}
            preventSkipAhead
          />

          <TitleAndDescriptionContainer>
            <Title>{getLocale('braveWalletCreatePasswordTitle')}</Title>
            <Description>{getLocale('braveWalletCreatePasswordDescription')}</Description>
          </TitleAndDescriptionContainer>

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
    </CenteredPageLayout>
  )
}

export default OnboardingCreatePassword
