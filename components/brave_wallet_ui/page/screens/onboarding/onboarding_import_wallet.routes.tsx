// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, Route, Switch } from 'react-router'

// types
import { WalletRoutes } from '../../../constants/types'

// selectors
import {
  useSafePageSelector,
  useSafeWalletSelector
} from '../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../common/selectors'
import { PageSelectors } from '../../selectors'

// components
import { OnboardingDisclosures } from './disclosures/disclosures'
import {
  ProtectedRoute //
} from '../../../components/shared/protected-routing/protected-route'
import {
  OnboardingImportOrRestoreWallet //
} from './import-or-restore-wallet/import-or-restore-wallet'
import {
  OnboardingRestoreFromRecoveryPhrase //
} from './restore-from-recovery-phrase/restore-from-recovery-phrase-v3'
import {
  OnboardingNetworkSelection //
} from './network_selection/onboarding_network_selection'
import {
  OnboardingRestoreFromExtension //
} from './restore-from-recovery-phrase/restore-from-extension'
import { OnboardingImportWalletType } from './components/onboarding-import-wallet-type/onboarding-import-wallet-type'

export const OnboardingImportWalletRoutes = () => {
  // redux
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const termsAcknowledged = useSafePageSelector(
    PageSelectors.walletTermsAcknowledged
  )

  // render
  return (
    <Switch>
      <Route
        path={WalletRoutes.OnboardingImportSelectWalletType}
        exact
      >
        <OnboardingImportWalletType />
      </Route>
      <Route
        path={WalletRoutes.OnboardingImportTerms}
        exact
      >
        <OnboardingDisclosures />
      </Route>

      <ProtectedRoute
        path={WalletRoutes.OnboardingImportOrRestore}
        exact
        requirement={termsAcknowledged}
        redirectRoute={WalletRoutes.OnboardingImportTerms}
      >
        <OnboardingImportOrRestoreWallet />
      </ProtectedRoute>

      <ProtectedRoute
        path={WalletRoutes.OnboardingImportNetworkSelection}
        exact
        requirement={termsAcknowledged}
        redirectRoute={WalletRoutes.OnboardingImportTerms}
      >
        <OnboardingNetworkSelection />
      </ProtectedRoute>

      {/* From seed phrase */}
      <ProtectedRoute
        path={WalletRoutes.OnboardingRestoreWallet}
        exact
        requirement={termsAcknowledged}
        redirectRoute={WalletRoutes.OnboardingImportTerms}
      >
        <OnboardingRestoreFromRecoveryPhrase />
      </ProtectedRoute>

      {/* From legacy crypto wallets extension */}
      <ProtectedRoute
        path={WalletRoutes.OnboardingImportLegacy}
        exact
        requirement={termsAcknowledged}
        redirectRoute={WalletRoutes.OnboardingImportTerms}
      >
        <OnboardingRestoreFromExtension restoreFrom='legacy' />
      </ProtectedRoute>

      {/* From MetaMask extension */}
      <ProtectedRoute
        path={WalletRoutes.OnboardingImportMetaMask}
        exact
        requirement={termsAcknowledged}
        redirectRoute={WalletRoutes.OnboardingImportTerms}
      >
        <OnboardingRestoreFromExtension restoreFrom='metamask' />
      </ProtectedRoute>

      <Redirect
        to={
          isWalletCreated
            ? WalletRoutes.OnboardingComplete
            : WalletRoutes.OnboardingWelcome
        }
      />
    </Switch>
  )
}
