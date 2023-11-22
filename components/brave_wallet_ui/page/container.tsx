// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { Redirect, Route, Switch } from 'react-router-dom'

// utils
import { getWalletLocationTitle } from '../utils/string-utils'
import { LOCAL_STORAGE_KEYS } from '../common/constants/local-storage-keys'
import {
  getInitialSessionRoute,
  isPersistableSessionRoute
} from '../utils/routes-utils'

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
import { SimplePageWrapper } from './screens/page-screen.styles'

// components
import { CryptoView } from '../components/desktop/views/crypto/index'
import { LockScreen } from '../components/desktop/lock-screen/index'
import {
  WalletPageLayout //
} from '../components/desktop/wallet-page-layout/index'
import {
  WalletSubViewLayout //
} from '../components/desktop/wallet-sub-view-layout/index'
import { Skeleton } from '../components/shared/loading-skeleton/styles'
import { OnboardingRoutes } from './screens/onboarding/onboarding.routes'
import {
  BackupWalletRoutes //
} from './screens/backup-wallet/backup-wallet.routes'
import { FundWalletScreen } from './screens/fund-wallet/fund-wallet'
import {
  OnboardingSuccess //
} from './screens/onboarding/onboarding-success/onboarding-success'
import { DepositFundsScreen } from './screens/fund-wallet/deposit-funds'
import { RestoreWallet } from './screens/restore-wallet/restore-wallet'
import { Swap } from './screens/swap/swap'
import { SendScreen } from './screens/send/send_screen/send_screen'
import { DevBitcoin } from './screens/dev-bitcoin/dev-bitcoin'
import {
  WalletPageWrapper //
} from '../components/desktop/wallet-page-wrapper/wallet-page-wrapper'
import {
  PageTitleHeader //
} from '../components/desktop/card-headers/page-title-header'

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
    return <Skeleton />
  }

  return (
    <>
      <Switch>
        {walletNotYetCreated ? (
          <WalletPageLayout>
            <WalletSubViewLayout>
              <OnboardingRoutes />
            </WalletSubViewLayout>
          </WalletPageLayout>
        ) : (
          // Post-onboarding flows
          <Switch>
            <Route
              path={WalletRoutes.OnboardingComplete}
              exact
            >
              <WalletPageLayout>
                <WalletSubViewLayout>
                  <OnboardingSuccess />
                </WalletSubViewLayout>
              </WalletPageLayout>
            </Route>

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

            {isWalletLocked && (
              <Route
                path={WalletRoutes.Unlock}
                exact={true}
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
              </Route>
            )}

            {!isWalletLocked && (
              <Route path={WalletRoutes.Backup}>
                <WalletPageLayout>
                  <WalletSubViewLayout>
                    <SimplePageWrapper>
                      <BackupWalletRoutes />
                    </SimplePageWrapper>
                  </WalletSubViewLayout>
                </WalletPageLayout>
              </Route>
            )}

            {!isWalletLocked && (
              <Route path={WalletRoutes.FundWalletPageStart}>
                <FundWalletScreen />
              </Route>
            )}

            {!isWalletLocked && (
              <Route path={WalletRoutes.DepositFundsPageStart}>
                <DepositFundsScreen />
              </Route>
            )}

            {!isWalletLocked && (
              <Route
                path={WalletRoutes.Swap}
                exact={true}
              >
                <WalletPageWrapper
                  hideHeader={true}
                  hideBackground={true}
                  cardHeader={<PageTitleHeader title={'braveWalletSwap'} />}
                >
                  <Swap />
                </WalletPageWrapper>
              </Route>
            )}

            {isBitcoinEnabled && !isWalletLocked && (
              <Route
                path={WalletRoutes.DevBitcoin}
                exact={true}
              >
                <DevBitcoin />
              </Route>
            )}

            {!isWalletLocked && (
              <Route
                path={WalletRoutes.Send}
                exact
              >
                <SendScreen />
              </Route>
            )}

            {!isWalletLocked && (
              <Route path={WalletRoutes.CryptoPage}>
                <CryptoView sessionRoute={sessionRoute} />
              </Route>
            )}

            {isWalletLocked && <Redirect to={WalletRoutes.Unlock} />}
            {!isWalletLocked && <Redirect to={WalletRoutes.PortfolioAssets} />}
          </Switch>
        )}
      </Switch>
    </>
  )
}

export default Container
