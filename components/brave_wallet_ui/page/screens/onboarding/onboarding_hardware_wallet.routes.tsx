// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, Route, Switch, useHistory } from 'react-router'

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
  OnboardingNetworkSelection //
} from './network_selection/onboarding_network_selection'
import {
  OnboardingConnectHardwareWallet //
} from './connect-hardware/onboarding_connect_hardware_wallet'
import {
  OnboardingCreatePassword //
} from './create_password/onboarding_create_password'
import {
  OnboardingImportWalletType //
} from './components/onboarding_import_wallet_type/import_wallet_type'
import {
  OnboardingImportHardwareWalletWelcome //
} from './import_hardware_wallet_welcome/import_hardware_wallet_welcome'
import {
  OnboardingSelectWalletDevice //
} from './select_wallet_device/onboarding_select_wallet_device'

export const OnboardingHardwareWalletRoutes = () => {
  // routing
  const history = useHistory()

  // redux
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const termsAcknowledged = useSafePageSelector(
    PageSelectors.walletTermsAcknowledged
  )

  // render
  return (
    <Switch>
      <Route
        path={WalletRoutes.OnboardingImportHardwareWalletWelcome}
        exact
      >
        <OnboardingImportHardwareWalletWelcome />
      </Route>
      <Route
        path={WalletRoutes.OnboardingImportSelectWalletType}
        exact
      >
        <OnboardingImportWalletType />
      </Route>
      <Route
        path={WalletRoutes.OnboardingHardwareWalletTerms}
        exact
      >
        <OnboardingDisclosures />
      </Route>

      <ProtectedRoute
        path={WalletRoutes.OnboardingHardwareWalletNetworkSelection}
        exact
        requirement={termsAcknowledged}
        redirectRoute={WalletRoutes.OnboardingHardwareWalletTerms}
      >
        <OnboardingNetworkSelection />
      </ProtectedRoute>

      <ProtectedRoute
        path={WalletRoutes.OnboardingHardwareWalletCreatePassword}
        requirement={termsAcknowledged}
        redirectRoute={WalletRoutes.OnboardingImportTerms}
      >
        <OnboardingCreatePassword
          onWalletCreated={() =>
            history.push(WalletRoutes.OnboardingHardwareWalletConnect)
          }
        />
      </ProtectedRoute>

      <ProtectedRoute
        path={WalletRoutes.OnboardingHardwareWalletConnectSelectDevice}
        requirement={isWalletCreated}
        redirectRoute={WalletRoutes.OnboardingImportTerms}
      >
        <OnboardingSelectWalletDevice />
      </ProtectedRoute>

      <ProtectedRoute
        path={WalletRoutes.OnboardingHardwareWalletConnect}
        requirement={isWalletCreated}
        redirectRoute={WalletRoutes.OnboardingImportTerms}
      >
        <OnboardingConnectHardwareWallet />
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
