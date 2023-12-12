// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { Redirect, Route, Switch } from 'react-router-dom'

import ProgressRing from '@brave/leo/react/progressRing'

// utils
import { getWalletLocationTitle } from '../utils/string-utils'
import {
  getInitialSessionRoute,
  isPersistableSessionRoute
} from '../utils/routes-utils'
import { LOCAL_STORAGE_KEYS } from '../common/constants/local-storage-keys'

// actions
import * as WalletPageActions from './actions/wallet_page_actions'

// selectors
import { WalletSelectors } from '../common/selectors'
import { PageSelectors } from './selectors'

// types
import { WalletRoutes } from '../constants/types'

// hooks
import {
  useSafePageSelector,
  useSafeWalletSelector
} from '../common/hooks/use-safe-selector'
import { useLocationPathName } from '../common/hooks/use-pathname'

// style
import 'emptykit.css'
import {
  FullScreenWrapper,
  SimplePageWrapper
} from './screens/page-screen.styles'

// components
import { LockScreen } from '../components/desktop/lock-screen/index'
import {
  WalletPageLayout //
} from '../components/desktop/wallet-page-layout/index'
import {
  WalletSubViewLayout //
} from '../components/desktop/wallet-sub-view-layout/index'
import { OnboardingRoutes } from './screens/onboarding/onboarding.routes'
import { DevBitcoin } from './screens/dev-bitcoin/dev-bitcoin'
import { RestoreWallet } from './screens/restore-wallet/restore-wallet'
import {
  WalletPageWrapper //
} from '../components/desktop/wallet-page-wrapper/wallet-page-wrapper'
import {
  ProtectedRoute //
} from '../components/shared/protected-routing/protected-route'
import { UnlockedWalletRoutes } from './router/unlocked_wallet_routes'
import { Swap } from './screens/swap/swap'
import { SendScreen } from './screens/send/send_screen/send_screen'

const initialSessionRoute = getInitialSessionRoute()

export const Container = () => {
  // routing
  const walletLocation = useLocationPathName()

  // redux
  const dispatch = useDispatch()

  // wallet selectors (safe)
  const hasInitialized = useSafeWalletSelector(WalletSelectors.hasInitialized)
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const isWalletLocked = useSafeWalletSelector(WalletSelectors.isWalletLocked)
  const isBitcoinEnabled = useSafeWalletSelector(
    WalletSelectors.isBitcoinEnabled
  )

  // page selectors (safe)
  const mnemonic = useSafePageSelector(PageSelectors.mnemonic)
  const setupStillInProgress = useSafePageSelector(
    PageSelectors.setupStillInProgress
  )

  // state
  const [sessionRoute, setSessionRoute] = React.useState(initialSessionRoute)

  // computed
  const walletNotYetCreated = !isWalletCreated || setupStillInProgress
  const defaultRedirect = walletNotYetCreated
    ? WalletRoutes.OnboardingWelcome
    : isWalletLocked
    ? WalletRoutes.Unlock
    : sessionRoute || WalletRoutes.PortfolioAssets

  // effects
  React.useEffect(() => {
    // update page title
    document.title = getWalletLocationTitle(walletLocation)

    // store the last url before wallet lock
    // so that we can return to that page after unlock
    if (isPersistableSessionRoute(walletLocation)) {
      window.localStorage.setItem(
        LOCAL_STORAGE_KEYS.SESSION_ROUTE,
        walletLocation
      )
      setSessionRoute(walletLocation)
    }
    // clean recovery phrase if not backing up or onboarding on route change
    if (
      mnemonic &&
      !walletLocation.includes(WalletRoutes.Backup) &&
      !walletLocation.includes(WalletRoutes.Onboarding)
    ) {
      dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic: '' }))
    }
  }, [walletLocation, mnemonic])

  // render
  if (!hasInitialized) {
    return (
      <FullScreenWrapper>
        <ProgressRing mode='indeterminate' />
      </FullScreenWrapper>
    )
  }

  return (
    <Switch>
      <ProtectedRoute
        path={WalletRoutes.Onboarding}
        requirement={walletNotYetCreated}
        redirectRoute={defaultRedirect}
      >
        <OnboardingRoutes />
      </ProtectedRoute>

      {/* Post-onboarding flows */}
      <Route
        path={WalletRoutes.Restore}
        exact={true}
      >
        <WalletPageLayout>
          <WalletSubViewLayout>
            <SimplePageWrapper>
              <RestoreWallet />
            </SimplePageWrapper>
          </WalletSubViewLayout>
        </WalletPageLayout>
      </Route>

      <ProtectedRoute
        path={WalletRoutes.Unlock}
        exact={true}
        requirement={isWalletLocked}
        redirectRoute={defaultRedirect}
      >
        <WalletPageWrapper
          wrapContentInBox={true}
          cardWidth={680}
          hideNav={true}
          hideHeaderMenu={true}
          noBorderRadius={true}
        >
          <LockScreen />
        </WalletPageWrapper>
      </ProtectedRoute>

      <ProtectedRoute
        path={WalletRoutes.Swap}
        requirement={!isWalletLocked && !walletNotYetCreated}
        redirectRoute={defaultRedirect}
        exact={true}
      >
        <Swap />
      </ProtectedRoute>

      <ProtectedRoute
        path={WalletRoutes.Send}
        requirement={!isWalletLocked && !walletNotYetCreated}
        redirectRoute={defaultRedirect}
        exact={true}
      >
        <SendScreen />
      </ProtectedRoute>

      <ProtectedRoute
        path={WalletRoutes.CryptoPage}
        requirement={!isWalletLocked && !walletNotYetCreated}
        redirectRoute={defaultRedirect}
      >
        <UnlockedWalletRoutes sessionRoute={sessionRoute} />
      </ProtectedRoute>

      <ProtectedRoute
        path={WalletRoutes.DevBitcoin}
        exact={true}
        requirement={
          !isWalletLocked && !walletNotYetCreated && isBitcoinEnabled
        }
        redirectRoute={defaultRedirect}
      >
        <DevBitcoin />
      </ProtectedRoute>

      <Redirect to={defaultRedirect} />
    </Switch>
  )
}

export default Container
