// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// types
import { PageState, WalletRoutes } from '../../../../../constants/types'

// components
import { StepsNavigation, StepsNavigationProps } from '../../../../../components/desktop/steps-navigation/steps-navigation'

//
// New wallet Steps
//
type OnboardingNewWalletSteps =
  | WalletRoutes.OnboardingBackupRecoveryPhrase
  | WalletRoutes.OnboardingBackupWallet
  | WalletRoutes.OnboardingComplete
  | WalletRoutes.OnboardingCreatePassword
  | WalletRoutes.OnboardingExplainRecoveryPhrase
  | WalletRoutes.OnboardingVerifyRecoveryPhrase
  | WalletRoutes.OnboardingWelcome

const NEW_WALLET_STEPS: OnboardingNewWalletSteps[] = [
  WalletRoutes.OnboardingWelcome,
  WalletRoutes.OnboardingCreatePassword,
  WalletRoutes.OnboardingExplainRecoveryPhrase,
  WalletRoutes.OnboardingBackupRecoveryPhrase,
  WalletRoutes.OnboardingVerifyRecoveryPhrase,
  WalletRoutes.OnboardingComplete
]

///
// Connect Hardware Steps
//
type OnboardingConnectHardwareWalletSteps =
  | WalletRoutes.OnboardingComplete
  | WalletRoutes.OnboardingCreatePassword
  | WalletRoutes.OnboardingWelcome

const CONNECT_HARDWARE_WALLET_STEPS: OnboardingConnectHardwareWalletSteps[] = [
  WalletRoutes.OnboardingWelcome,
  WalletRoutes.OnboardingCreatePassword,
  WalletRoutes.OnboardingComplete
]

interface OnboardingNewWalletStepsNavigationProps extends Omit<StepsNavigationProps<OnboardingNewWalletSteps>, 'steps'> {}

export const OnboardingNewWalletStepsNavigation = (
  props: OnboardingNewWalletStepsNavigationProps
) => {
  const isHardwareOnboarding = useSelector(({ page }: { page: PageState }) => page.isHardwareOnboarding)

  return <StepsNavigation
    {...props}
    steps={isHardwareOnboarding ? CONNECT_HARDWARE_WALLET_STEPS : NEW_WALLET_STEPS}
  />
}
