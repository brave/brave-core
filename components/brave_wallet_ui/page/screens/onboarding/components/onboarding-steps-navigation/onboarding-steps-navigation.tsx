// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// routes
import { WalletRoutes } from '../../../../../constants/types'

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
  WalletRoutes.OnboardingCreatePassword,
  WalletRoutes.OnboardingExplainRecoveryPhrase,
  WalletRoutes.OnboardingBackupRecoveryPhrase,
  WalletRoutes.OnboardingVerifyRecoveryPhrase,
  WalletRoutes.OnboardingComplete
]

interface OnboardingNewWalletStepsNavigationProps extends Omit<StepsNavigationProps<OnboardingNewWalletSteps>, 'steps'> {}

export const OnboardingNewWalletStepsNavigation = ({
  currentStep,
  preventSkipAhead,
  goBackUrl,
  onSkip
}: OnboardingNewWalletStepsNavigationProps) => {
  return <StepsNavigation
    steps={NEW_WALLET_STEPS}
    goBackUrl={goBackUrl}
    currentStep={currentStep}
    preventSkipAhead={preventSkipAhead}
    onSkip={onSkip}
  />
}

//
// Import Wallet Steps
//
type OnboardingImportWalletSteps =
  | WalletRoutes.OnboardingComplete
  | WalletRoutes.OnboardingImportCryptoWallets
  | WalletRoutes.OnboardingImportMetaMask
  | WalletRoutes.OnboardingWelcome

const IMPORT_WALLET_STEPS: OnboardingImportWalletSteps[] = [
  WalletRoutes.OnboardingWelcome,
  WalletRoutes.OnboardingImportCryptoWallets,
  WalletRoutes.OnboardingImportMetaMask,
  WalletRoutes.OnboardingComplete
]

interface OnboardingImportWalletStepsNavigationProps extends Omit<StepsNavigationProps<OnboardingImportWalletSteps>, 'steps'> {}

export const OnboardingImportWalletStepsNavigation = ({
  currentStep,
  preventSkipAhead,
  goBackUrl
}: OnboardingImportWalletStepsNavigationProps) => {
  return <StepsNavigation
    steps={IMPORT_WALLET_STEPS}
    goBackUrl={goBackUrl}
    currentStep={currentStep}
    preventSkipAhead={preventSkipAhead}
  />
}

//
// Restore Wallet Steps
//
type OnboardingRestoreWalletSteps =
  | WalletRoutes.OnboardingComplete
  | WalletRoutes.OnboardingRestoreWallet
  | WalletRoutes.OnboardingWelcome

const RESTORE_WALLET_STEPS: OnboardingRestoreWalletSteps[] = [
  WalletRoutes.OnboardingWelcome,
  WalletRoutes.OnboardingRestoreWallet,
  WalletRoutes.OnboardingComplete
]

interface OnboardingRestoreWalletStepsNavigationProps extends Omit<StepsNavigationProps<OnboardingImportWalletSteps>, 'steps'> {}

export const OnboardingRestoreWalletStepsNavigation = ({
  currentStep,
  preventSkipAhead,
  goBackUrl,
  onSkip
}: OnboardingRestoreWalletStepsNavigationProps) => {
  return <StepsNavigation
    steps={RESTORE_WALLET_STEPS}
    goBackUrl={goBackUrl}
    currentStep={currentStep}
    preventSkipAhead={preventSkipAhead}
    onSkip={onSkip}
  />
}
