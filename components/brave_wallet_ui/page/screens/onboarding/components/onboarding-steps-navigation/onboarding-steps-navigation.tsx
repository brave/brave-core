// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// types
import { WalletRoutes } from '../../../../../constants/types'

// utils
import { useLocationPathName } from '../../../../../common/hooks/use-pathname'
import {
  getOnboardingImportTypeFromPath,
  getOnboardingTypeFromPath
} from '../../../../../utils/routes-utils'

// components
import {
  StepsNavigation,
  StepsNavigationProps
} from '../../../../../components/desktop/steps-navigation/steps-navigation'

const NEW_WALLET_STEPS: WalletRoutes[] = [
  WalletRoutes.OnboardingWelcome,
  WalletRoutes.OnboardingNewWalletTerms,
  WalletRoutes.OnboardingNewWalletNetworkSelection,
  WalletRoutes.OnboardingNewWalletCreatePassword,
  WalletRoutes.OnboardingExplainRecoveryPhrase,
  WalletRoutes.OnboardingBackupRecoveryPhrase,
  WalletRoutes.OnboardingVerifyRecoveryPhrase,
  WalletRoutes.OnboardingComplete
]

const RESTORE_WALLET_STEPS: WalletRoutes[] = [
  WalletRoutes.OnboardingWelcome,
  WalletRoutes.OnboardingImportTerms,
  WalletRoutes.OnboardingImportNetworkSelection,
  WalletRoutes.OnboardingImportOrRestore,
  WalletRoutes.OnboardingRestoreWallet,
  WalletRoutes.OnboardingComplete
]

const IMPORT_METAMASK_STEPS: WalletRoutes[] = [
  WalletRoutes.OnboardingWelcome,
  WalletRoutes.OnboardingImportTerms,
  WalletRoutes.OnboardingImportNetworkSelection,
  WalletRoutes.OnboardingImportOrRestore,
  WalletRoutes.OnboardingImportMetaMask,
  WalletRoutes.OnboardingComplete
]

const IMPORT_LEGACY_STEPS: WalletRoutes[] = [
  WalletRoutes.OnboardingWelcome,
  WalletRoutes.OnboardingImportTerms,
  WalletRoutes.OnboardingImportNetworkSelection,
  WalletRoutes.OnboardingImportOrRestore,
  WalletRoutes.OnboardingImportLegacy,
  WalletRoutes.OnboardingComplete
]

const CONNECT_HARDWARE_WALLET_STEPS: WalletRoutes[] = [
  WalletRoutes.OnboardingWelcome,
  WalletRoutes.OnboardingHardwareWalletTerms,
  WalletRoutes.OnboardingHardwareWalletNetworkSelection,
  WalletRoutes.OnboardingHardwareWalletCreatePassword,
  WalletRoutes.OnboardingHardwareWalletConnect,
  WalletRoutes.OnboardingComplete
]

interface OnboardingNewWalletStepsNavigationProps
  extends Omit<StepsNavigationProps<WalletRoutes>, 'steps' | 'currentStep'> {}

export const OnboardingStepsNavigation = (
  props: OnboardingNewWalletStepsNavigationProps
) => {
  // routing
  const path = useLocationPathName()
  const onboardingType = getOnboardingTypeFromPath(path)
  const importType = getOnboardingImportTypeFromPath(path)

  return (
    <StepsNavigation
      {...props}
      currentStep={path}
      steps={
        onboardingType === 'new'
          ? NEW_WALLET_STEPS
          : onboardingType === 'hardware'
          ? CONNECT_HARDWARE_WALLET_STEPS
          : importType === 'seed'
          ? RESTORE_WALLET_STEPS
          : importType === 'metamask'
          ? IMPORT_METAMASK_STEPS
          : IMPORT_LEGACY_STEPS
      }
    />
  )
}
