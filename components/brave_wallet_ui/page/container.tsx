// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { Redirect, Route, Switch, useHistory, useLocation } from 'react-router-dom'

// actions
import * as WalletPageActions from './actions/wallet_page_actions'
import * as WalletActions from '../common/actions/wallet_actions'

// types
import {
  BraveWallet,
  BuySendSwapTypes,
  PageState,
  UpdateAccountNamePayloadType,
  WalletAccountType,
  WalletRoutes,
  WalletState
} from '../constants/types'

// style
import 'emptykit.css'
import '../../../ui/webui/resources/fonts/poppins.css'
import '../../../ui/webui/resources/fonts/muli.css'
import { OnboardingWrapper, WalletWidgetStandIn } from '../stories/style'

// components
import { CryptoView, LockScreen, OnboardingRestore, WalletPageLayout, WalletSubViewLayout } from '../components/desktop'
import BuySendSwap from '../stories/screens/buy-send-swap'
import Onboarding from '../stories/screens/onboarding'
import BackupWallet from '../stories/screens/backup-wallet'
import { SweepstakesBanner } from '../components/desktop/sweepstakes-banner'
import { Skeleton } from '../components/shared/loading-skeleton/styles'

// Hooks
import { useAssets, useLib } from '../common/hooks'
import ProtectedRoute from '../components/shared/protected-routing/protected-route'

// Utils
import { getRampAssetSymbol } from '../utils/asset-utils'

export const Container = () => {
  // routing
  let history = useHistory()
  const { pathname: walletLocation } = useLocation()

  // redux
  const dispatch = useDispatch()

  const {
    isWalletCreated,
    isWalletLocked,
    isWalletBackedUp,
    hasIncorrectPassword,
    selectedNetwork,
    selectedAccount,
    hasInitialized,
    defaultWallet,
    isMetaMaskInstalled
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  const setupStillInProgress = useSelector(({ page }: { page: PageState }) => page.setupStillInProgress)

  // state
  const [sessionRoute, setSessionRoute] = React.useState<string | undefined>(undefined)
  const [inputValue, setInputValue] = React.useState<string>('')
  const [buyAmount, setBuyAmount] = React.useState('')
  const [selectedWidgetTab, setSelectedWidgetTab] = React.useState<BuySendSwapTypes>('buy')
  const [selectedBuyOption, setSelectedBuyOption] = React.useState<BraveWallet.OnRampProvider>(BraveWallet.OnRampProvider.kRamp)

  // custom hooks
  const { buyAssetOptions, wyreAssetOptions, rampAssetOptions } = useAssets()
  const { getBuyAssetUrl } = useLib()

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

  const onSetBuyAmount = (value: string) => {
    setBuyAmount(value)
  }

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

  const onHideBackup = React.useCallback(() => {
    dispatch(WalletPageActions.showRecoveryPhrase(false))
    history.goBack()
  }, [])

  const handlePasswordChanged = React.useCallback((value: string) => {
    setInputValue(value)
    if (hasIncorrectPassword) {
      dispatch(WalletActions.hasIncorrectPassword(false))
    }
  }, [hasIncorrectPassword])

  const onSubmitBuy = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    const buyAsset = selectedBuyOption === BraveWallet.OnRampProvider.kRamp ? { ...asset, symbol: getRampAssetSymbol(asset) } : asset
    getBuyAssetUrl({
      asset: buyAsset,
      onRampProvider: selectedBuyOption,
      chainId: selectedNetwork.chainId,
      address: selectedAccount.address,
      amount: buyAmount
    })
    .then(url => window.open(url, '_blank'))
    .catch(e => console.error(e))
  }, [getBuyAssetUrl, selectedBuyOption, selectedNetwork, selectedAccount, buyAmount])

  const onUpdateAccountName = React.useCallback((payload: UpdateAccountNamePayloadType): { success: boolean } => {
    const result = dispatch(WalletPageActions.updateAccountName(payload))
    return result ? { success: true } : { success: false }
  }, [])

  const onViewPrivateKey = React.useCallback((address: string, isDefault: boolean, coin: BraveWallet.CoinType) => {
    dispatch(WalletPageActions.viewPrivateKey({ address, isDefault, coin }))
  }, [])

  const onDoneViewingPrivateKey = React.useCallback(() => {
    dispatch(WalletPageActions.doneViewingPrivateKey())
  }, [])

  const onOpenWalletSettings = React.useCallback(() => {
    dispatch(WalletPageActions.openWalletSettings())
  }, [])

  // computed
  const walletNotYetCreated = (!isWalletCreated || setupStillInProgress) && walletLocation !== WalletRoutes.Restore
  const hideMainComponents =
    (isWalletCreated && !setupStillInProgress) &&
    !isWalletLocked &&
    walletLocation !== WalletRoutes.Backup

  const walletUnlockNeeded = (isWalletCreated && !setupStillInProgress) && isWalletLocked &&
    walletLocation !== WalletRoutes.Unlock &&
    walletLocation !== WalletRoutes.Restore

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
      !walletLocation.includes(WalletRoutes.Onboarding) &&
      walletLocation !== WalletRoutes.Unlock &&
      walletLocation !== WalletRoutes.Restore // can be accessed from unlock screen
    ) {
      setSessionRoute(walletLocation)
    }
  }, [walletLocation])

  // render
  if (!hasInitialized) {
    return <Skeleton />
  }

  return (
    <WalletPageLayout>
      {/* <SideNav
        navList={NavOptions}
        selectedButton={view}
        onSubmit={navigateTo}
      /> */}
      <WalletSubViewLayout>

        <Switch>
          <ProtectedRoute
            path={WalletRoutes.Onboarding}
            requirement={setupStillInProgress || walletNotYetCreated}
            redirectRoute={sessionRoute as WalletRoutes || WalletRoutes.Portfolio}
          >
            <Onboarding />
          </ProtectedRoute>

          <ProtectedRoute
            path={WalletRoutes.Restore}
            requirement={isWalletLocked}
            redirectRoute={sessionRoute as WalletRoutes || WalletRoutes.Unlock}
            exact={true}
          >
            <OnboardingWrapper>
              <OnboardingRestore />
            </OnboardingWrapper>
          </ProtectedRoute>

          <ProtectedRoute
            path={WalletRoutes.Unlock}
            redirectRoute={sessionRoute as WalletRoutes || WalletRoutes.Portfolio}
            requirement={isWalletLocked && !walletNotYetCreated}
            exact={true}
          >
            <OnboardingWrapper>
              <LockScreen
                value={inputValue}
                onSubmit={unlockWallet}
                disabled={inputValue === ''}
                onPasswordChanged={handlePasswordChanged}
                hasPasswordError={hasIncorrectPassword}
                onShowRestore={onToggleShowRestore}
              />
            </OnboardingWrapper>
          </ProtectedRoute>

          {/* If wallet is not yet created will Route to Onboarding */}
          {walletNotYetCreated && <Redirect to={WalletRoutes.Onboarding} />}

          {/* Redirect to unlock screen if needed */}
          {walletUnlockNeeded && <Redirect to={WalletRoutes.Unlock} />}

          <Route path={WalletRoutes.Backup} exact={true}>
            <OnboardingWrapper>
              <BackupWallet
                isOnboarding={false}
                onCancel={onHideBackup}
              />
            </OnboardingWrapper>
          </Route>

          {/* Default */}
          <ProtectedRoute
            path={WalletRoutes.CryptoPage}
            requirement={!walletUnlockNeeded}
            redirectRoute={WalletRoutes.Unlock}
          >
            <CryptoView
              needsBackup={!isWalletBackedUp}
              onUpdateAccountName={onUpdateAccountName}
              onDoneViewingPrivateKey={onDoneViewingPrivateKey}
              onViewPrivateKey={onViewPrivateKey}
              defaultWallet={defaultWallet}
              onOpenWalletSettings={onOpenWalletSettings}
              isMetaMaskInstalled={isMetaMaskInstalled}
            />
          </ProtectedRoute>

          <Redirect to={WalletRoutes.Portfolio} />
        </Switch>
      </WalletSubViewLayout>
      {hideMainComponents &&
        <WalletWidgetStandIn>
          <BuySendSwap
            selectedTab={selectedWidgetTab}
            buyAmount={buyAmount}
            buyAssetOptions={buyAssetOptions}
            onSetBuyAmount={onSetBuyAmount}
            onSubmitBuy={onSubmitBuy}
            onSelectAccount={onSelectAccount}
            onSelectTab={setSelectedWidgetTab}
            selectedBuyOption={selectedBuyOption}
            onSelectBuyOption={setSelectedBuyOption}
            wyreAssetOptions={wyreAssetOptions}
            rampAssetOptions={rampAssetOptions}
          />
          <SweepstakesBanner />
        </WalletWidgetStandIn>
      }
    </WalletPageLayout>
  )
}

export default Container
