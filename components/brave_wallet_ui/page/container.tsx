// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { Redirect, Route, Switch, useHistory } from 'react-router-dom'

// utils
import { getWalletLocationTitle } from '../utils/string-utils'
import { LOCAL_STORAGE_KEYS } from '../common/constants/local-storage-keys'
import {
  getInitialSessionRoute,
  isPersistableSessionRoute
} from '../utils/routes-utils'

// actions
import * as WalletPageActions from './actions/wallet_page_actions'
import * as WalletActions from '../common/actions/wallet_actions'

// selectors
import { UISelectors, WalletSelectors } from '../common/selectors'
import { PageSelectors } from './selectors'

// types
import { WalletOrigin, WalletRoutes, WalletState } from '../constants/types'

// hooks
import {
  useSafePageSelector,
  useSafeUISelector,
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
  let history = useHistory()
  const walletLocation = useLocationPathName()

  // redux
  const dispatch = useDispatch()
  const isBitcoinEnabled = useSelector(
    ({ wallet }: { wallet: WalletState }) => wallet.isBitcoinEnabled
  )

  // wallet selectors (safe)
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const isWalletLocked = useSafeWalletSelector(WalletSelectors.isWalletLocked)
  const isWalletBackedUp = useSafeWalletSelector(
    WalletSelectors.isWalletBackedUp
  )
  const hasIncorrectPassword = useSafeWalletSelector(
    WalletSelectors.hasIncorrectPassword
  )
  const hasInitialized = useSafeWalletSelector(WalletSelectors.hasInitialized)
  const defaultEthereumWallet = useSafeWalletSelector(
    WalletSelectors.defaultEthereumWallet
  )
  const defaultSolanaWallet = useSafeWalletSelector(
    WalletSelectors.defaultSolanaWallet
  )
  const isMetaMaskInstalled = useSafeWalletSelector(
    WalletSelectors.isMetaMaskInstalled
  )

  // page selectors (safe)
  const setupStillInProgress = useSafePageSelector(
    PageSelectors.setupStillInProgress
  )

  // UI Selectors (safe)
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // state
  const [sessionRoute, setSessionRoute] = React.useState(initialSessionRoute)
  const [inputValue, setInputValue] = React.useState<string>('')

  // methods
  const onToggleShowRestore = React.useCallback(() => {
    if (walletLocation === WalletRoutes.Restore) {
      // If a user has not yet created a wallet and clicks Restore
      // from the panel, we need to route to onboarding if they click back.
      if (!isWalletCreated) {
        history.push(WalletRoutes.Onboarding)
        return
      }
      // If a user has created a wallet and clicks Restore from the panel while
      // the wallet is locked, we need to route to unlock if they click back.
      if (isWalletCreated && isWalletLocked) {
        history.push(WalletRoutes.Unlock)
      }
      return
    }
    if (isPanel) {
      chrome.tabs.create(
        { url: `${WalletOrigin}${WalletRoutes.Restore}` },
        () => {
          if (chrome.runtime.lastError) {
            console.error(
              'tabs.create failed: ' + //
                chrome.runtime.lastError.message
            )
          }
        }
      )
      return
    }
    history.push(WalletRoutes.Restore)
  }, [walletLocation, isPanel])

  const unlockWallet = React.useCallback(() => {
    dispatch(WalletActions.unlockWallet({ password: inputValue }))
    setInputValue('')
    if (sessionRoute) {
      history.push(sessionRoute)
    } else {
      history.push(WalletRoutes.PortfolioAssets)
    }
  }, [inputValue, sessionRoute])

  const handlePasswordChanged = React.useCallback(
    (value: string) => {
      setInputValue(value)
      if (hasIncorrectPassword) {
        dispatch(WalletActions.hasIncorrectPassword(false))
      }
    },
    [hasIncorrectPassword]
  )

  const onOpenWalletSettings = React.useCallback(() => {
    dispatch(WalletPageActions.openWalletSettings())
  }, [])

  // computed
  const walletNotYetCreated = !isWalletCreated || setupStillInProgress

  // effects
  React.useEffect(() => {
    if (hasIncorrectPassword) {
      // Clear incorrect password
      setInputValue('')
    }
  }, [hasIncorrectPassword])

  React.useEffect(() => {
    // store the last url before wallet lock
    // so that we can return to that page after unlock
    if (isPersistableSessionRoute(walletLocation)) {
      window.localStorage.setItem(
        LOCAL_STORAGE_KEYS.SESSION_ROUTE,
        walletLocation
      )
      setSessionRoute(walletLocation)
    }
  }, [walletLocation, isWalletCreated])

  React.useEffect(() => {
    const toobarElement = document.getElementById('toolbar')
    const rootElement = document.getElementById('root')
    if (toobarElement && rootElement) {
      if (
        walletLocation === WalletRoutes.Swap ||
        walletLocation === WalletRoutes.SendPageStart ||
        walletLocation.includes(WalletRoutes.DepositFundsPageStart) ||
        walletLocation.includes(WalletRoutes.FundWalletPageStart) ||
        walletLocation.includes(WalletRoutes.LocalIpfsNode) ||
        walletLocation.includes(WalletRoutes.InspectNfts) ||
        walletLocation.includes(WalletRoutes.PortfolioAssets) ||
        walletLocation.includes(WalletRoutes.PortfolioNFTs) ||
        walletLocation.includes(WalletRoutes.PortfolioNFTAsset) ||
        walletLocation.includes(WalletRoutes.Market) ||
        walletLocation.includes(WalletRoutes.Activity) ||
        walletLocation.includes(WalletRoutes.Accounts) ||
        walletLocation === WalletRoutes.Unlock
      ) {
        toobarElement.hidden = true
        rootElement.style.setProperty('min-height', '100vh')
        return
      }
      toobarElement.hidden = false
      rootElement.style.setProperty('min-height', 'calc(100vh - 56px)')
    }
  }, [walletLocation])

  React.useEffect(() => {
    document.title = getWalletLocationTitle(walletLocation)
  }, [walletLocation])

  React.useEffect(() => {
    // clean recovery phrase if not backing up or onboarding on route change
    if (
      !walletLocation.includes(WalletRoutes.Backup) &&
      !walletLocation.includes(WalletRoutes.Onboarding)
    ) {
      dispatch(WalletPageActions.recoveryWordsAvailable({ mnemonic: '' }))
    }
  }, [walletLocation])

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
                  <LockScreen
                    value={inputValue}
                    onSubmit={unlockWallet}
                    disabled={inputValue === ''}
                    onPasswordChanged={handlePasswordChanged}
                    hasPasswordError={hasIncorrectPassword}
                    onShowRestore={onToggleShowRestore}
                  />
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
                path={WalletRoutes.SendPage}
                exact
              >
                <SendScreen />
              </Route>
            )}

            {!isWalletLocked && (
              <Route path={WalletRoutes.CryptoPage}>
                <CryptoView
                  needsBackup={!isWalletBackedUp}
                  defaultEthereumWallet={defaultEthereumWallet}
                  defaultSolanaWallet={defaultSolanaWallet}
                  onOpenWalletSettings={onOpenWalletSettings}
                  isMetaMaskInstalled={isMetaMaskInstalled}
                  sessionRoute={sessionRoute}
                />
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
