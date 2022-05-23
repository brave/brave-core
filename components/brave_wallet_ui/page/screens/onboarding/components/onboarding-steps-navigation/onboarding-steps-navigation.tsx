// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// routes
import { WalletRoutes } from '../../../../../constants/types'

// components
import { StepsNavigation, StepsNavigationProps } from '../../../../../components/desktop/steps-navigation/steps-navigation'

type OnboardingSteps =
  | WalletRoutes.OnboardingBackupRecoveryPhrase
  | WalletRoutes.OnboardingBackupWallet
  | WalletRoutes.OnboardingComplete
  | WalletRoutes.OnboardingCreatePassword
  | WalletRoutes.OnboardingExplainRecoveryPhrase
  | WalletRoutes.OnboardingImportCryptoWallets
  | WalletRoutes.OnboardingImportMetaMask
  | WalletRoutes.OnboardingVerifyRecoveryPhrase
  | WalletRoutes.OnboardingWelcome

const STEPS: OnboardingSteps[] = [
  WalletRoutes.OnboardingCreatePassword,
  WalletRoutes.OnboardingExplainRecoveryPhrase,
  WalletRoutes.OnboardingBackupRecoveryPhrase,
  WalletRoutes.OnboardingVerifyRecoveryPhrase,
  WalletRoutes.OnboardingComplete
]

interface Props extends Omit<StepsNavigationProps<OnboardingSteps>, 'steps'> {}

export const OnboardingStepsNavigation = ({
  currentStep,
  preventSkipAhead,
  goBack
}: Props) => {
  return <StepsNavigation
    goBack={goBack}
    steps={STEPS}
    currentStep={currentStep}
    preventSkipAhead={preventSkipAhead}
  />
}

export default OnboardingStepsNavigation
