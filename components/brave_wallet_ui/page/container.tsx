// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { Redirect, Route, Switch, useHistory, useLocation } from 'react-router-dom'

// utils
import { getWalletLocationTitle } from '../utils/string-utils'

// actions
import * as WalletPageActions from './actions/wallet_page_actions'
import * as WalletActions from '../common/actions/wallet_actions'

// selectors
import { WalletSelectors } from '../common/selectors'
import { PageSelectors } from './selectors'

// types
import {
  WalletRoutes
} from '../constants/types'

// hooks
import { useSafePageSelector, useSafeWalletSelector } from '../common/hooks/use-safe-selector'

// style
import 'emptykit.css'
import {
  SimplePageWrapper
} from './screens/page-screen.styles'

// components
import { CryptoView, LockScreen, WalletPageLayout, WalletSubViewLayout } from '../components/desktop'
import { Skeleton } from '../components/shared/loading-skeleton/styles'
import { OnboardingRoutes } from './screens/onboarding/onboarding.routes'
import { BackupWalletRoutes } from './screens/backup-wallet/backup-wallet.routes'
import { FundWalletScreen } from './screens/fund-wallet/fund-wallet'
import { OnboardingSuccess } from './screens/onboarding/onboarding-success/onboarding-success'
import { DepositFundsScreen } from './screens/fund-wallet/deposit-funds'
import { RestoreWallet } from './screens/restore-wallet/restore-wallet'
import { Swap } from './screens/swap/swap'
import { SendScreen } from './screens/send/send-page/send-screen'
import {
  WalletPageWrapper
} from '../components/desktop/wallet-page-wrapper/wallet-page-wrapper'

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

  // computed
  const walletNotYetCreated = (!isWalletCreated || setupStillInProgress)

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
      walletLocation.includes(WalletRoutes.DepositFundsPageStart) ||
      walletLocation.includes(WalletRoutes.FundWalletPageStart) ||
      walletLocation.includes(WalletRoutes.Portfolio) ||
      walletLocation.includes(WalletRoutes.Market) ||
      walletLocation.includes(WalletRoutes.Nfts) ||
      walletLocation.includes(WalletRoutes.Swap) ||
      walletLocation.includes(WalletRoutes.Send) ||
      walletLocation.includes(WalletRoutes.LocalIpfsNode ||
        walletLocation.includes(WalletRoutes.InspectNfts))
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
        walletLocation === WalletRoutes.Send ||
        walletLocation.includes(WalletRoutes.DepositFundsPage) ||
        walletLocation.includes(WalletRoutes.FundWalletPage) ||
        walletLocation.includes(WalletRoutes.LocalIpfsNode) ||
        walletLocation.includes(WalletRoutes.InspectNfts) ||
        walletLocation.includes(WalletRoutes.Portfolio) ||
        walletLocation.includes(WalletRoutes.Market) ||
        walletLocation.includes(WalletRoutes.Nfts) ||
        walletLocation.includes(WalletRoutes.Activity) ||
        walletLocation.includes(WalletRoutes.Accounts)
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

        {walletNotYetCreated
          ?
          <WalletPageLayout>
            <WalletSubViewLayout>
              <OnboardingRoutes />
            </WalletSubViewLayout>
          </WalletPageLayout>

          // Post-onboarding flows
          : <Switch>

            <Route path={WalletRoutes.OnboardingComplete} exact>
              <WalletPageLayout>
                <WalletSubViewLayout>
                  <OnboardingSuccess />
                </WalletSubViewLayout>
              </WalletPageLayout>
            </Route>

            <Route path={WalletRoutes.Restore} exact={true}>
              <WalletPageLayout>
                <WalletSubViewLayout>
                  <SimplePageWrapper>
                    <RestoreWallet />
                  </SimplePageWrapper>
                </WalletSubViewLayout>
              </WalletPageLayout>
            </Route>

            {isWalletLocked &&
              <Route path={WalletRoutes.Unlock} exact={true}>
                <WalletPageLayout>
                  <WalletSubViewLayout>
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
                  </WalletSubViewLayout>
                </WalletPageLayout>
              </Route>
            }

            {!isWalletLocked &&
              <Route path={WalletRoutes.Backup}>
                <WalletPageLayout>
                  <WalletSubViewLayout>
                    <SimplePageWrapper>
                      <BackupWalletRoutes />
                    </SimplePageWrapper>
                  </WalletSubViewLayout>
                </WalletPageLayout>
              </Route>
            }

            {!isWalletLocked &&
              <Route path={WalletRoutes.FundWalletPage} exact>
                <WalletPageWrapper
                  wrapContentInBox={true}
                  cardWidth={456}
                  cardOverflow='visible'
                >
                  <FundWalletScreen />
                </WalletPageWrapper>
              </Route>
            }

            {!isWalletLocked &&
              <Route path={WalletRoutes.DepositFundsPage} exact>
                <WalletPageWrapper
                  wrapContentInBox={true}
                  cardWidth={456}
                  cardOverflow='visible'
                >
                  <DepositFundsScreen />
                </WalletPageWrapper>
              </Route>
            }

            {!isWalletLocked &&
              <Route path={WalletRoutes.Swap} exact={true}>
                <WalletPageWrapper hideBackground={true}>
                  <Swap />
                </WalletPageWrapper>
              </Route>
            }

            {!isWalletLocked &&
              <Route path={WalletRoutes.Send} exact={true}>
                <WalletPageWrapper hideBackground={true}>
                  <SendScreen />
                </WalletPageWrapper>
              </Route>
            }

            {!isWalletLocked &&
              <Route path={WalletRoutes.CryptoPage}>
                <WalletPageWrapper
                  noPadding={
                    walletLocation === WalletRoutes.LocalIpfsNode ||
                    walletLocation === WalletRoutes.InspectNfts
                  }
                  wrapContentInBox={
                    walletLocation !== WalletRoutes.LocalIpfsNode &&
                    walletLocation !== WalletRoutes.InspectNfts
                  }
                  cardOverflow={
                    walletLocation === WalletRoutes.Portfolio ||
                      walletLocation === WalletRoutes.Activity ||
                      walletLocation === WalletRoutes.Nfts
                      ? 'visible'
                      : 'hidden'
                  }
                >
                  <CryptoView
                    needsBackup={!isWalletBackedUp}
                    defaultEthereumWallet={defaultEthereumWallet}
                    defaultSolanaWallet={defaultSolanaWallet}
                    onOpenWalletSettings={onOpenWalletSettings}
                    isMetaMaskInstalled={isMetaMaskInstalled}
                    sessionRoute={sessionRoute}
                  />

                </WalletPageWrapper>
              </Route>
            }

            {isWalletLocked && <Redirect to={WalletRoutes.Unlock} />}
            {!isWalletLocked && <Redirect to={WalletRoutes.Portfolio} />}
          </Switch>
        }
      </Switch>
    </>
  )
}

export default Container
