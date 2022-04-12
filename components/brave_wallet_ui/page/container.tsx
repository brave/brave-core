// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { connect } from 'react-redux'
import { bindActionCreators, Dispatch } from 'redux'
import { Route, Switch, useHistory, useLocation } from 'react-router-dom'

import * as WalletPageActions from './actions/wallet_page_actions'
import * as WalletActions from '../common/actions/wallet_actions'
import store from './store'

import 'emptykit.css'
import '../../../ui/webui/resources/fonts/poppins.css'
import '../../../ui/webui/resources/fonts/muli.css'

import { OnboardingWrapper, WalletWidgetStandIn } from '../stories/style'
import { CryptoView, LockScreen, OnboardingRestore, WalletPageLayout, WalletSubViewLayout } from '../components/desktop'
import {
  BraveWallet,
  BuySendSwapTypes,
  PageState,
  UpdateAccountNamePayloadType,
  WalletAccountType,
  WalletPageState,
  WalletRoutes,
  WalletState
} from '../constants/types'
import BuySendSwap from '../stories/screens/buy-send-swap'
import Onboarding from '../stories/screens/onboarding'
import BackupWallet from '../stories/screens/backup-wallet'
import { SweepstakesBanner } from '../components/desktop/sweepstakes-banner'

// Utils
import { GetBuyOrFaucetUrl } from '../utils/buy-asset-url'

import {
  getBalance,
  onConnectHardwareWallet
} from '../common/async/lib'

// Hooks
import { useAssets } from '../common/hooks'

type Props = {
  wallet: WalletState
  page: PageState
  walletPageActions: typeof WalletPageActions
  walletActions: typeof WalletActions
}

function Container (props: Props) {
  let history = useHistory()
  const { pathname: walletLocation } = useLocation()
  // Wallet Props
  const {
    isFilecoinEnabled,
    isSolanaEnabled,
    isWalletCreated,
    isWalletLocked,
    isWalletBackedUp,
    hasIncorrectPassword,
    accounts,
    networkList,
    transactions,
    selectedNetwork,
    selectedAccount,
    hasInitialized,
    defaultWallet,
    isMetaMaskInstalled,
    defaultCurrencies,
    fullTokenList,
    userVisibleTokensInfo
  } = props.wallet

  // Page Props
  const {
    mnemonic,
    selectedTimeline,
    selectedAsset,
    setupStillInProgress,
    privateKey,
    importAccountError,
    showAddModal
  } = props.page

  // const [view, setView] = React.useState<NavTypes>('crypto')
  const [inputValue, setInputValue] = React.useState<string>('')
  const [buyAmount, setBuyAmount] = React.useState('')
  const [selectedWidgetTab, setSelectedWidgetTab] = React.useState<BuySendSwapTypes>('buy')
  const [showVisibleAssetsModal, setShowVisibleAssetsModal] = React.useState<boolean>(false)
  const [sessionRoute, setSessionRoute] = React.useState<string | undefined>(undefined)

  const { buyAssetOptions } = useAssets()

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
    props.walletActions.selectAccount(account)
  }

  const onBackupWallet = () => {
    props.walletPageActions.walletBackupComplete()
    history.goBack()
  }

  const unlockWallet = () => {
    props.walletActions.unlockWallet({ password: inputValue })
    setInputValue('')
  }

  const lockWallet = () => {
    props.walletActions.lockWallet()
  }

  const onShowBackup = () => {
    props.walletPageActions.showRecoveryPhrase(true)
    history.push(WalletRoutes.Backup)
  }

  const onHideBackup = () => {
    props.walletPageActions.showRecoveryPhrase(false)
    history.goBack()
  }

  const handlePasswordChanged = (value: string) => {
    setInputValue(value)
    if (hasIncorrectPassword) {
      props.walletActions.hasIncorrectPassword(false)
    }
  }

  const recoveryPhrase = React.useMemo(() => {
    return (mnemonic || '').split(' ')
  }, [mnemonic])

  const onSelectAsset = (asset: BraveWallet.BlockchainToken) => {
    props.walletPageActions.selectAsset({ asset: asset, timeFrame: selectedTimeline })
  }

  const onShowAddModal = () => {
    props.walletPageActions.setShowAddModal(true)
  }

  const onHideAddModal = () => {
    props.walletPageActions.setShowAddModal(false)
  }

  const onCreateAccount = (name: string, coin: BraveWallet.CoinType) => {
    const created = props.walletPageActions.addAccount({ accountName: name, coin: coin })
    if (walletLocation.includes(WalletRoutes.Accounts)) {
      history.push(WalletRoutes.Accounts)
    }
    if (created) {
      onHideAddModal()
    }
  }

  const onSubmitBuy = (asset: BraveWallet.BlockchainToken) => {
    GetBuyOrFaucetUrl(selectedNetwork.chainId, asset, selectedAccount, buyAmount)
      .then(url => window.open(url, '_blank'))
      .catch(e => console.error(e))
  }

  const onAddHardwareAccounts = (selected: BraveWallet.HardwareWalletAccount[]) => {
    props.walletPageActions.addHardwareAccounts(selected)
  }

  const onImportAccount = (accountName: string, privateKey: string, coin: BraveWallet.CoinType) => {
    props.walletPageActions.importAccount({ accountName, privateKey, coin })
  }

  const onImportFilecoinAccount = (accountName: string, privateKey: string, network: string) => {
    props.walletPageActions.importFilecoinAccount({ accountName, privateKey, network })
  }

  const onImportAccountFromJson = (accountName: string, password: string, json: string) => {
    props.walletPageActions.importAccountFromJson({ accountName, password, json })
  }

  const onSetImportAccountError = (hasError: boolean) => {
    props.walletPageActions.setImportAccountError(hasError)
  }

  const onRemoveAccount = (address: string, hardware: boolean, coin: BraveWallet.CoinType) => {
    if (hardware) {
      props.walletPageActions.removeHardwareAccount({ address, coin })
      return
    }
    props.walletPageActions.removeImportedAccount({ address, coin })
  }

  const onUpdateAccountName = (payload: UpdateAccountNamePayloadType): { success: boolean } => {
    const result = props.walletPageActions.updateAccountName(payload)
    return result ? { success: true } : { success: false }
  }

  const fetchFullTokenList = () => {
    props.walletActions.getAllTokensList()
  }

  const onViewPrivateKey = (address: string, isDefault: boolean, coin: BraveWallet.CoinType) => {
    props.walletPageActions.viewPrivateKey({ address, isDefault, coin })
  }

  const onDoneViewingPrivateKey = () => {
    props.walletPageActions.doneViewingPrivateKey()
  }

  const checkWalletsToImport = () => {
    props.walletPageActions.checkWalletsToImport()
  }

  const onOpenWalletSettings = () => {
    props.walletPageActions.openWalletSettings()
  }

  const onShowVisibleAssetsModal = (showModal: boolean) => {
    if (showModal) {
      if (fullTokenList.length === 0) {
        fetchFullTokenList()
      }
      history.push(`${WalletRoutes.AddAssetModal}`)
    } else {
      history.push(`${WalletRoutes.Portfolio}`)
    }
    setShowVisibleAssetsModal(showModal)
  }

  React.useEffect(() => {
    // Creates a list of Accepted Portfolio Routes
    const acceptedPortfolioRoutes = userVisibleTokensInfo.map((token) => {
      if (token.contractAddress === '') {
        return `${WalletRoutes.Portfolio}/${token.symbol}`
      }
      return `${WalletRoutes.Portfolio}/${token.contractAddress}`
    })
    // Creates a list of Accepted Account Routes
    const acceptedAccountRoutes = accounts.map((account) => {
      return `${WalletRoutes.Accounts}/${account.address}`
    })

    const allAcceptedRoutes = [
      WalletRoutes.Backup,
      WalletRoutes.Accounts,
      WalletRoutes.AddAccountModal,
      WalletRoutes.AddAssetModal,
      WalletRoutes.Portfolio,
      ...acceptedPortfolioRoutes,
      ...acceptedAccountRoutes
    ]

    if (allAcceptedRoutes.includes(walletLocation)) {
      setSessionRoute(walletLocation)
    }

    if (!hasInitialized) {
      return
    }
    // If wallet is not yet created will Route to Onboarding
    if ((!isWalletCreated || setupStillInProgress) && walletLocation !== WalletRoutes.Restore) {
      checkWalletsToImport()
      history.push(WalletRoutes.Onboarding)
      // If wallet is created will Route to login
    } else if (isWalletLocked && walletLocation !== WalletRoutes.Restore) {
      history.push(WalletRoutes.Unlock)
      // Allowed Private Routes if a wallet is unlocked else will redirect back to Portfolio
    } else if (
      !isWalletLocked &&
      hasInitialized &&
      !allAcceptedRoutes.includes(walletLocation) &&
      acceptedAccountRoutes.length !== 0 &&
      acceptedPortfolioRoutes.length !== 0
    ) {
      if (sessionRoute) {
        history.push(sessionRoute)
      } else {
        history.push(WalletRoutes.Portfolio)
      }
    }
  }, [
    walletLocation,
    isWalletCreated,
    isWalletLocked,
    hasInitialized,
    setupStillInProgress,
    selectedAsset,
    userVisibleTokensInfo,
    accounts
  ])

  React.useEffect(() => {
    if (hasIncorrectPassword) {
      // Clear incorrect password
      setInputValue('')
    }
  }, [hasIncorrectPassword])

  const hideMainComponents = (isWalletCreated && !setupStillInProgress) && !isWalletLocked && walletLocation !== WalletRoutes.Backup

  return (
    <WalletPageLayout>
      {/* <SideNav
        navList={NavOptions}
        selectedButton={view}
        onSubmit={navigateTo}
      /> */}
      <Switch>
        <WalletSubViewLayout>
          <Route path={WalletRoutes.Restore} exact={true}>
            {isWalletLocked &&
              <OnboardingWrapper>
                <OnboardingRestore />
              </OnboardingWrapper>
            }
          </Route>
          <Route path={WalletRoutes.Onboarding} exact={true}>
            <Onboarding />
          </Route>
          <Route path={WalletRoutes.Unlock} exact={true}>
            {isWalletLocked &&
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
            }
          </Route>
          <Route path={WalletRoutes.Backup} exact={true}>
            <OnboardingWrapper>
              <BackupWallet
                isOnboarding={false}
                onCancel={onHideBackup}
                onSubmit={onBackupWallet}
                recoveryPhrase={recoveryPhrase}
              />
            </OnboardingWrapper>
          </Route>
          <Route path={WalletRoutes.CryptoPage} exact={true}>
            {hideMainComponents &&
              <CryptoView
                defaultCurrencies={defaultCurrencies}
                onLockWallet={lockWallet}
                needsBackup={!isWalletBackedUp}
                onShowBackup={onShowBackup}
                accounts={accounts}
                networkList={networkList}
                onSelectAsset={onSelectAsset}
                transactions={transactions}
                onConnectHardwareWallet={onConnectHardwareWallet}
                onCreateAccount={onCreateAccount}
                onImportAccount={onImportAccount}
                onImportFilecoinAccount={onImportFilecoinAccount}
                isFilecoinEnabled={isFilecoinEnabled}
                isSolanaEnabled={isSolanaEnabled}
                showAddModal={showAddModal}
                onHideAddModal={onHideAddModal}
                onUpdateAccountName={onUpdateAccountName}
                selectedNetwork={selectedNetwork}
                onRemoveAccount={onRemoveAccount}
                privateKey={privateKey ?? ''}
                onDoneViewingPrivateKey={onDoneViewingPrivateKey}
                onViewPrivateKey={onViewPrivateKey}
                onImportAccountFromJson={onImportAccountFromJson}
                onSetImportError={onSetImportAccountError}
                hasImportError={importAccountError}
                onAddHardwareAccounts={onAddHardwareAccounts}
                userVisibleTokensInfo={userVisibleTokensInfo}
                getBalance={getBalance}
                defaultWallet={defaultWallet}
                onOpenWalletSettings={onOpenWalletSettings}
                onShowAddModal={onShowAddModal}
                isMetaMaskInstalled={isMetaMaskInstalled}
                onShowVisibleAssetsModal={onShowVisibleAssetsModal}
                showVisibleAssetsModal={showVisibleAssetsModal}
              />
            }
          </Route>
        </WalletSubViewLayout>
      </Switch>
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
          />
          <SweepstakesBanner />
        </WalletWidgetStandIn>
      }
    </WalletPageLayout>
  )
}

function mapStateToProps (state: WalletPageState): Partial<Props> {
  return {
    page: state.page,
    wallet: state.wallet
  }
}

function mapDispatchToProps (dispatch: Dispatch): Partial<Props> {
  return {
    walletPageActions: bindActionCreators(WalletPageActions, store.dispatch.bind(store)),
    walletActions: bindActionCreators(WalletActions, store.dispatch.bind(store))
  }
}

export default connect(mapStateToProps, mapDispatchToProps)(Container)
