// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, Route, Switch, useHistory } from 'react-router'

// types
import { WalletRoutes } from '../../../constants/types'

// selectors
import { PageSelectors } from '../../selectors'

// hooks
import { useSafePageSelector } from '../../../common/hooks/use-safe-selector'
import { useGetWalletInfoQuery } from '../../../common/slices/api.slice'

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
} from './connect-hardware/onboarding-connect-hardware-wallet'
import {
  OnboardingCreatePassword //
} from './create-password/onboarding-create-password'

export const OnboardingHardwareWalletRoutes = () => {
  // routing
  const history = useHistory()

  // queries
  const { data: walletInfo } = useGetWalletInfoQuery()
  const isWalletCreated = walletInfo?.isWalletCreated ?? false

  // redux
  const termsAcknowledged = useSafePageSelector(
    PageSelectors.walletTermsAcknowledged
  )

  // render
  return (
    <Switch>
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
