// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// constants
import { BraveWallet } from '../../../../constants/types'

// utils
import { getLocale } from '../../../../../common/locale'
import {
  useCreateWalletMutation,
  useReportOnboardingActionMutation
} from '../../../../common/slices/api.slice'
import {
  useSafeWalletSelector //
} from '../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../common/selectors'

// components
import {
  NavButton //
} from '../../../../components/extension/buttons/nav-button/index'
import {
  NewPasswordInput,
  NewPasswordValues
} from '../../../../components/shared/password-input/new-password-input'
import {
  OnboardingStepsNavigation //
} from '../components/onboarding-steps-navigation/onboarding-steps-navigation'
import { CenteredPageLayout } from '../../../../components/desktop/centered-page-layout/centered-page-layout'
import { CreatingWallet } from '../creating_wallet/creating_wallet'

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
  onWalletCreated: () => void
}

export const OnboardingCreatePassword = ({
  onWalletCreated
}: OnboardingCreatePasswordProps) => {
  // redux
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)

  // state
  const [isValid, setIsValid] = React.useState(false)
  const [password, setPassword] = React.useState('')

  // mutations
  const [createWallet, { isLoading: isCreatingWallet }] =
    useCreateWalletMutation()
  const [report] = useReportOnboardingActionMutation()

  // methods
  const nextStep = React.useCallback(async () => {
    if (!isValid) {
      return
    }
    // Note: intentionally not using unwrapped value
    // results are returned before other redux actions complete
    await createWallet({ password }).unwrap()
  }, [isValid, createWallet, password])

  const handlePasswordChange = React.useCallback(
    ({ isValid, password }: NewPasswordValues) => {
      setPassword(password)
      setIsValid(isValid)
    },
    []
  )

  // effects
  React.useEffect(() => {
    report(BraveWallet.OnboardingAction.LegalAndPassword)
  }, [report])

  React.useEffect(() => {
    // wait for redux before redirecting
    // otherwise, the restricted routes in the router will not be available
    if (!isCreatingWallet && isWalletCreated) {
      onWalletCreated()
    }
  }, [isWalletCreated, onWalletCreated, isCreatingWallet])

  if (isCreatingWallet) {
    return <CreatingWallet />
  }

  // render
  return (
    <CenteredPageLayout>
      <MainWrapper>
        <StyledWrapper>
          <OnboardingStepsNavigation preventSkipAhead />

          <TitleAndDescriptionContainer>
            <Title>{getLocale('braveWalletCreatePasswordTitle')}</Title>
            <Description>
              {getLocale('braveWalletCreatePasswordDescription')}
            </Description>
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
