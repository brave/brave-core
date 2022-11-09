// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { Redirect, Route, Switch, useHistory, useLocation } from 'react-router-dom'

// utils
import { getLocale } from '$web-common/locale'
import { getWalletLocationTitle } from '../utils/string-utils'

// actions
import * as WalletPageActions from './actions/wallet_page_actions'
import * as WalletActions from '../common/actions/wallet_actions'

// selectors
import { WalletSelectors } from '../common/selectors'
import { PageSelectors } from './selectors'

// types
import {
  BuySendSwapTypes,
  WalletAccountType,
  WalletRoutes
} from '../constants/types'

// hooks
import { useSafePageSelector, useSafeWalletSelector } from '../common/hooks/use-safe-selector'

// style
import 'emptykit.css'
import {
  ButtonText,
  FeatureRequestButton,
  IdeaButtonIcon,
  SimplePageWrapper,
  WalletWidgetStandIn
} from './screens/page-screen.styles'

// components
import { CryptoView, LockScreen, WalletPageLayout, WalletSubViewLayout } from '../components/desktop'
import { Skeleton } from '../components/shared/loading-skeleton/styles'
import BuySendSwap from '../stories/screens/buy-send-swap'
import { OnboardingRoutes } from './screens/onboarding/onboarding.routes'
import { BackupWalletRoutes } from './screens/backup-wallet/backup-wallet.routes'
import { SweepstakesBanner } from '../components/desktop/sweepstakes-banner'
import { FundWalletScreen } from './screens/fund-wallet/fund-wallet'
import { OnboardingSuccess } from './screens/onboarding/onboarding-success/onboarding-success'
import { DepositFundsScreen } from './screens/fund-wallet/deposit-funds'
import { RestoreWallet } from './screens/restore-wallet/restore-wallet'
import { Swap } from './screens/swap/swap'

const featureRequestUrl = 'https://community.brave.com/tags/c/wallet/131/feature-request'

export const Container = () => {
  // routing
  let history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // redux
  const dispatch = useDispatch()

  // wallet selectors (safe)
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const isWalletLocked = useSafeWalletSelector(WalletSelectors.isWalletLocked)
  const isWalletBackedUp = useSafeWalletSelector(WalletSelectors.isWalletBackedUp)
  const hasIncorrectPassword = useSafeWalletSelector(WalletSelectors.hasIncorrectPassword)
  const hasInitialized = useSafeWalletSelector(WalletSelectors.hasInitialized)
  const defaultEthereumWallet = useSafeWalletSelector(WalletSelectors.defaultEthereumWallet)
  const defaultSolanaWallet = useSafeWalletSelector(WalletSelectors.defaultSolanaWallet)
  const isMetaMaskInstalled = useSafeWalletSelector(WalletSelectors.isMetaMaskInstalled)

  // page selectors (safe)
  const setupStillInProgress = useSafePageSelector(PageSelectors.setupStillInProgress)

  // state
  const [sessionRoute, setSessionRoute] = React.useState<string | undefined>(undefined)
  const [inputValue, setInputValue] = React.useState<string>('')
  const [selectedWidgetTab, setSelectedWidgetTab] = React.useState<BuySendSwapTypes>('buy')

  // methods
  const onToggleShowRestore = React.useCallback(() => {
    if (walletLocation === WalletRoutes.Restore) {
      // If a user has not yet created a wallet and clicks Restore
      // from the panel, we need to route to onboarding if they click back.
      if (!isWalletCreated) {
        history.push(WalletRoutes.Onboarding)
        return
      }
      // If a user has created a wallet and clicks Restore from the panel
      // while the wallet is locked, we need to route to unlock if they click back.
      if (isWalletCreated && isWalletLocked) {
        history.push(WalletRoutes.Unlock)
      }
    } else {
      history.push(WalletRoutes.Restore)
    }
  }, [walletLocation])

  const onSelectAccount = (account: WalletAccountType) => {
    dispatch(WalletActions.selectAccount(account))
  }

  const unlockWallet = React.useCallback(() => {
    dispatch(WalletActions.unlockWallet({ password: inputValue }))
    setInputValue('')
    if (sessionRoute) {
      history.push(sessionRoute)
    } else {
      history.push(WalletRoutes.Portfolio)
    }
  }, [inputValue, sessionRoute])

  const handlePasswordChanged = React.useCallback((value: string) => {
    setInputValue(value)
    if (hasIncorrectPassword) {
      dispatch(WalletActions.hasIncorrectPassword(false))
    }
  }, [hasIncorrectPassword])

  const onOpenWalletSettings = React.useCallback(() => {
    dispatch(WalletPageActions.openWalletSettings())
  }, [])

  const onClickFeatureRequestButton = React.useCallback(() => {
    chrome.tabs.create({ url: featureRequestUrl }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }, [])

  // computed
  const walletNotYetCreated = (!isWalletCreated || setupStillInProgress)

  const showBuySendSwapSidebar =
    isWalletCreated && !isWalletLocked &&
    (
      walletLocation.includes(WalletRoutes.Portfolio) ||
      walletLocation.includes(WalletRoutes.Accounts) ||
      walletLocation.includes(WalletRoutes.Market) ||
      walletLocation.includes(WalletRoutes.Nfts)
    )

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
    if (
      walletLocation.includes(WalletRoutes.Accounts) ||
      walletLocation.includes(WalletRoutes.Backup) ||
      walletLocation.includes(WalletRoutes.DepositFundsPage) ||
      walletLocation.includes(WalletRoutes.FundWalletPage) ||
      walletLocation.includes(WalletRoutes.Portfolio) ||
      walletLocation.includes(WalletRoutes.Market) ||
      walletLocation.includes(WalletRoutes.Nfts) ||
      walletLocation.includes(WalletRoutes.Swap)
    ) {
      setSessionRoute(walletLocation)
    }
  }, [walletLocation, isWalletCreated])

  React.useEffect(() => {
    const toobarElement = document.getElementById('toolbar')
    const rootElement = document.getElementById('root')
    if (toobarElement && rootElement) {
      if (
        walletLocation === WalletRoutes.Swap ||
        walletLocation.includes(WalletRoutes.DepositFundsPage) ||
        walletLocation.includes(WalletRoutes.FundWalletPage)
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
    <WalletPageLayout maintainWidth={walletLocation === WalletRoutes.Swap}>
      <WalletSubViewLayout noPadding={walletLocation === WalletRoutes.Swap}>

        <Switch>

          {walletNotYetCreated
            ? <OnboardingRoutes />

            // Post-onboarding flows
            : <Switch>

              <Route path={WalletRoutes.OnboardingComplete} exact>
                <OnboardingSuccess />
              </Route>

              <Route path={WalletRoutes.FundWalletPage} exact>
                <FundWalletScreen />
              </Route>

              <Route path={WalletRoutes.DepositFundsPage} exact>
                <DepositFundsScreen />
              </Route>

              <Route path={WalletRoutes.Restore} exact={true}>
                <SimplePageWrapper>
                  <RestoreWallet />
                </SimplePageWrapper>
              </Route>

              {isWalletLocked &&
                <Route path={WalletRoutes.Unlock} exact={true}>
                  <SimplePageWrapper>
                    <LockScreen
                      value={inputValue}
                      onSubmit={unlockWallet}
                      disabled={inputValue === ''}
                      onPasswordChanged={handlePasswordChanged}
                      hasPasswordError={hasIncorrectPassword}
                      onShowRestore={onToggleShowRestore}
                    />
                  </SimplePageWrapper>
                </Route>
              }

              {!isWalletLocked &&
                <Route path={WalletRoutes.Swap} exact={true}>
                  <Swap />
                </Route>
              }

              {!isWalletLocked &&
                <Route path={WalletRoutes.Backup}>
                  <SimplePageWrapper>
                    <BackupWalletRoutes />
                  </SimplePageWrapper>
                </Route>
              }

              {!isWalletLocked &&
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
              }

              {isWalletLocked && <Redirect to={WalletRoutes.Unlock} />}
              {!isWalletLocked && <Redirect to={WalletRoutes.Portfolio} />}

            </Switch>
          }
        </Switch>
      </WalletSubViewLayout>

      {showBuySendSwapSidebar &&
        <WalletWidgetStandIn>
          <BuySendSwap
            selectedTab={selectedWidgetTab}
            onSelectAccount={onSelectAccount}
            onSelectTab={setSelectedWidgetTab}
          />
          <SweepstakesBanner />
        </WalletWidgetStandIn>
      }

      {!isWalletLocked && walletLocation !== WalletRoutes.Swap &&
        <FeatureRequestButton onClick={onClickFeatureRequestButton}>
          <IdeaButtonIcon />
          <ButtonText>{getLocale('braveWalletRequestFeatureButtonText')}</ButtonText>
        </FeatureRequestButton>
      }
    </WalletPageLayout>
  )
}

export default Container
