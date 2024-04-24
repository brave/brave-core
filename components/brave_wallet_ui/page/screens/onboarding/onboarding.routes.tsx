// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, Route, Switch } from 'react-router'
import { useDispatch } from 'react-redux'

// types
import { WalletRoutes } from '../../../constants/types'

// redux
import { PageSelectors } from '../../selectors'
import { WalletPageActions } from '../../actions'

// hooks
import { useGetWalletInfoQuery } from '../../../common/slices/api.slice'
import { useSafePageSelector } from '../../../common/hooks/use-safe-selector'

// components
import {
  WalletPageLayout //
} from '../../../components/desktop/wallet-page-layout'
import {
  WalletSubViewLayout //
} from '../../../components/desktop/wallet-sub-view-layout'
import {
  ProtectedRoute //
} from '../../../components/shared/protected-routing/protected-route'
import { OnboardingSuccess } from './onboarding-success/onboarding-success'
import { OnboardingNewWalletRoutes } from './onboarding_new_wallet.routes'
import { OnboardingWelcome } from './welcome/onboarding-welcome'
import { OnboardingImportWalletRoutes } from './onboarding_import_wallet.routes'
import {
  OnboardingHardwareWalletRoutes //
} from './onboarding_hardware_wallet.routes'

export const OnboardingRoutes = () => {
  // queries
  const { data: walletInfo } = useGetWalletInfoQuery()
  const isWalletCreated = walletInfo?.isWalletCreated ?? false

  // redux
  const dispatch = useDispatch()
  const setupStillInProgress = useSafePageSelector(
    PageSelectors.setupStillInProgress
  )

  // effects
  React.useEffect(() => {
    // start wallet setup
    if (!setupStillInProgress) {
      dispatch(WalletPageActions.walletSetupComplete(false))
    }
  }, [setupStillInProgress, dispatch])

  // render
  return (
    <WalletPageLayout>
      <WalletSubViewLayout>
        <Switch>
          <ProtectedRoute
            path={WalletRoutes.OnboardingWelcome}
            requirement={!isWalletCreated}
            redirectRoute={WalletRoutes.OnboardingComplete}
          >
            <OnboardingWelcome />
          </ProtectedRoute>

          <Route path={WalletRoutes.OnboardingNewWalletStart}>
            <OnboardingNewWalletRoutes />
          </Route>

          <Route path={WalletRoutes.OnboardingImportStart}>
            <OnboardingImportWalletRoutes />
          </Route>

          <Route path={WalletRoutes.OnboardingHardwareWalletStart}>
            <OnboardingHardwareWalletRoutes />
          </Route>

          <ProtectedRoute
            path={WalletRoutes.OnboardingComplete}
            exact
            requirement={isWalletCreated}
            redirectRoute={WalletRoutes.OnboardingWelcome}
          >
            <OnboardingSuccess />
          </ProtectedRoute>

          <Redirect
            to={
              isWalletCreated
                ? WalletRoutes.OnboardingComplete
                : WalletRoutes.OnboardingWelcome
            }
          />
        </Switch>
      </WalletSubViewLayout>
    </WalletPageLayout>
  )
}

export default OnboardingRoutes
