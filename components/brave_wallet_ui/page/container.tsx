// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { connect } from 'react-redux'
import { bindActionCreators, Dispatch } from 'redux'

import * as WalletPageActions from './actions/wallet_page_actions'
import * as WalletActions from '../common/actions/wallet_actions'
import store from './store'

import 'emptykit.css'
import '../../../ui/webui/resources/fonts/poppins.css'
import '../../../ui/webui/resources/fonts/muli.css'

import { WalletWidgetStandIn } from '../stories/style'
import {
  // SideNav,
  WalletPageLayout,
  WalletSubViewLayout,
  CryptoView,
  LockScreen
} from '../components/desktop'
import {
  // NavTypes,
  WalletState,
  PageState,
  WalletPageState,
  AssetPriceTimeframe,
  AssetOptionType,
  NetworkOptionsType,
  OrderTypes,
  UserAccountType,
  SlippagePresetObjectType,
  ExpirationPresetObjectType,
  ToOrFromType
} from '../constants/types'
import { mockUserAccounts } from '../stories/mock-data/user-accounts'
// import { NavOptions } from '../options/side-nav-options'
import BuySendSwap from '../stories/screens/buy-send-swap'
import Onboarding from '../stories/screens/onboarding'
import BackupWallet from '../stories/screens/backup-wallet'
import { formatePrices } from '../utils/format-prices'
import { convertMojoTimeToJS } from '../utils/mojo-time'
import { AssetOptions } from '../options/asset-options'
import { NetworkOptions } from '../options/network-options'
import { SlippagePresetOptions } from '../options/slippage-preset-options'
import { ExpirationPresetOptions } from '../options/expiration-preset-options'

type Props = {
  wallet: WalletState
  page: PageState
  walletPageActions: typeof WalletPageActions
  walletActions: typeof WalletActions
}

function Container (props: Props) {
  // Wallet Props
  const {
    isWalletCreated,
    isWalletLocked,
    isWalletBackedUp,
    hasIncorrectPassword,
    accounts,
    transactions
  } = props.wallet

  // Page Props
  const {
    showRecoveryPhrase,
    invalidMnemonic,
    mnemonic,
    selectedTimeline,
    selectedAsset,
    selectedUSDAssetPrice,
    selectedBTCAssetPrice,
    selectedAssetPriceHistory,
    portfolioPriceHistory,
    userAssets,
    isFetchingPriceHistory
  } = props.page

  // const [view, setView] = React.useState<NavTypes>('crypto')
  const [inputValue, setInputValue] = React.useState<string>('')
  const [showAddModal, setShowAddModal] = React.useState<boolean>(false)
  const [exchangeRate, setExchangeRate] = React.useState('')
  const [toAddress, setToAddress] = React.useState('')
  const [sendAmount, setSendAmount] = React.useState('')
  const [fromAmount, setFromAmount] = React.useState('')
  const [toAmount, setToAmount] = React.useState('')
  const [slippageTolerance, setSlippageTolerance] = React.useState<SlippagePresetObjectType>(SlippagePresetOptions[0])
  const [orderExpiration, setOrderExpiration] = React.useState<ExpirationPresetObjectType>(ExpirationPresetOptions[0])
  const [orderType, setOrderType] = React.useState<OrderTypes>('market')

  const onSetToAddress = (value: string) => {
    setToAddress(value)
  }

  const onSetFromAmount = (value: string) => {
    setFromAmount(value)
  }

  const onSetSendAmount = (value: string) => {
    setSendAmount(value)
  }

  const onSetToAmount = (value: string) => {
    setToAmount(value)
  }

  const onSetExchangeRate = (value: string) => {
    setExchangeRate(value)
  }

  const onSelectExpiration = (expiration: ExpirationPresetObjectType) => {
    setOrderExpiration(expiration)
  }

  const onSelectSlippageTolerance = (slippage: SlippagePresetObjectType) => {
    setSlippageTolerance(slippage)
  }

  const onSelectPresetAmount = (percent: number) => {
    // 0 Will be replaced with selected from asset's Balance
    // once we are able to get balances
    const amount = 0 * percent
    setFromAmount(amount.toString())
  }

  const onToggleOrderType = () => {
    if (orderType === 'market') {
      setOrderType('limit')
    } else {
      setOrderType('market')
    }
  }

  // TODO (DOUGLAS): SelectedAccount needs to be stored in reducer
  // and setSelectedAccount as a page action
  const initialValue = React.useMemo(() => {
    if (isWalletLocked) {
      return mockUserAccounts[0]
    }
    return accounts[0]
  }, [invalidMnemonic])
  const [selectedAccount, setSelectedAccount] = React.useState<UserAccountType>(initialValue)
  const onSelectAccount = (account: UserAccountType) => {
    setSelectedAccount(account)
  }

  // TODO (DOUGLAS): This needs to be set up in the Reducer in a future PR
  // When the network Controller is Setup
  const [selectedNetwork, setSelectedNetwork] = React.useState<NetworkOptionsType>(NetworkOptions[0])
  const onSelectNetwork = (network: NetworkOptionsType) => {
    setSelectedNetwork(network)
  }

  // TODO (DOUGLAS): This needs to be set up in the Reducer in a future PR
  const [fromAsset, setFromAsset] = React.useState<AssetOptionType>(AssetOptions[0])
  const [toAsset, setToAsset] = React.useState<AssetOptionType>(AssetOptions[1])
  const onSelectTransactAsset = (asset: AssetOptionType, toOrFrom: ToOrFromType) => {
    if (toOrFrom === 'from') {
      setFromAsset(asset)
    } else {
      setToAsset(asset)
    }
  }
  const flipSwapAssets = () => {
    setFromAsset(toAsset)
    setToAsset(fromAsset)
  }

  // In the future these will be actual paths
  // for example wallet/rewards
  // const navigateTo = (path: NavTypes) => {
  //   setView(path)
  // }

  const completeWalletSetup = (recoveryVerified: boolean) => {
    if (recoveryVerified) {
      props.walletPageActions.walletBackupComplete()
    }
    props.walletPageActions.walletSetupComplete()
  }

  const onBackupWallet = () => {
    props.walletPageActions.walletBackupComplete()
  }

  const restoreWallet = (mnemonic: string, password: string) => {
    props.walletPageActions.restoreWallet({ mnemonic, password })
  }

  const passwordProvided = (password: string) => {
    props.walletPageActions.createWallet({ password })
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
  }

  const onHideBackup = () => {
    props.walletPageActions.showRecoveryPhrase(false)
  }

  const handlePasswordChanged = (value: string) => {
    setInputValue(value)
    if (hasIncorrectPassword) {
      props.walletActions.hasIncorrectPassword(false)
    }
  }

  const restorError = React.useMemo(() => {
    if (invalidMnemonic) {
      setTimeout(function () { props.walletPageActions.hasMnemonicError(false) }, 5000)
      return true
    }
    return false
  }, [invalidMnemonic])

  const recoveryPhrase = (mnemonic || '').split(' ')

  const onChangeTimeline = (timeline: AssetPriceTimeframe) => {
    if (selectedAsset) {
      props.walletPageActions.selectAsset({ asset: selectedAsset, timeFrame: timeline })
    }
  }

  // This will scrape all of the user's accounts and combine the asset balances for a single asset
  const fullAssetBalance = (asset: AssetOptionType) => {
    const newList = accounts.filter((account) => account.asset.includes(asset.symbol.toLowerCase()))
    const amounts = newList.map((account) => {
      return account.balance
    })
    const grandTotal = amounts.reduce(function (a, b) {
      return a + b
    }, 0)
    return grandTotal
  }

  // This will scrape all of the user's accounts and combine the fiat value for a single asset
  const fullAssetFiatBalance = (asset: AssetOptionType) => {
    const newList = accounts.filter((account) => account.asset.includes(asset.symbol.toLowerCase()))
    const amounts = newList.map((account) => {
      return Number(account.fiatBalance)
    })
    const grandTotal = amounts.reduce(function (a, b) {
      return a + b
    }, 0)
    return grandTotal
  }

  // This looks at the users asset list and returns the full balance for each asset
  const userAssetList = React.useMemo(() => {
    const newList = AssetOptions.filter((asset) => userAssets.includes(asset.id))
    return newList.map((asset) => {
      return {
        asset: asset,
        assetBalance: fullAssetBalance(asset),
        fiatBalance: fullAssetFiatBalance(asset)
      }
    })
  }, [userAssets, accounts])

  const onSelectAsset = (asset: AssetOptionType) => {
    props.walletPageActions.selectAsset({ asset: asset, timeFrame: selectedTimeline })
  }

  // This will scrape all of the user's accounts and combine the fiat value for every asset
  const fullPortfolioBalance = React.useMemo(() => {
    const amountList = userAssetList.map((item) => {
      return fullAssetFiatBalance(item.asset)
    })
    const grandTotal = amountList.reduce(function (a, b) {
      return a + b
    }, 0)
    return formatePrices(grandTotal)
  }, [userAssetList])

  const formatedPriceHistory = React.useMemo(() => {
    const formated = selectedAssetPriceHistory.map((obj) => {
      return {
        date: convertMojoTimeToJS(obj.date),
        close: Number(obj.price)
      }
    })
    return formated
  }, [selectedAssetPriceHistory])

  const onToggleAddModal = () => {
    setShowAddModal(!showAddModal)
  }

  const onCreateAccount = (name: string) => {
    const created = props.walletPageActions.addAccountToWallet({ accountName: name })
    if (created) {
      onToggleAddModal()
    }
  }

  const onConnectHardwareWallet = () => {
    // TODO (DOUGLAS): Add logic to connect a hardware wallet
  }

  const onImportAccount = () => {
    // TODO (DOUGLAS): Add logic to import a secondary account
  }

  const onUpdateAccountName = () => {
    // TODO (DOUGLAS): Need to add logic to update and Existing Account Name
  }

  const onUpdateWatchList = () => {
    // TODO (DOUGLAS): Need to persist a AssetWatchList and add logic to update
    // the AssetWatchList
  }

  const onSubmitSwap = () => {
    // TODO (DOUGLAS): logic Here to submit a swap transaction
  }

  const onSubmitSend = () => {
    // TODO (DOUGLAS): logic Here to submit a send transaction
  }

  const renderWallet = React.useMemo(() => {
    if (!isWalletCreated) {
      return (
        <Onboarding
          recoveryPhrase={recoveryPhrase}
          onPasswordProvided={passwordProvided}
          onSubmit={completeWalletSetup}
          onRestore={restoreWallet}
          hasRestoreError={restorError}
        />
      )
    } else {
      return (
        <>
          {isWalletLocked ? (
            <LockScreen
              onSubmit={unlockWallet}
              disabled={inputValue === ''}
              onPasswordChanged={handlePasswordChanged}
              hasPasswordError={hasIncorrectPassword}
            />
          ) : (
            <>
              {showRecoveryPhrase ? (
                <BackupWallet
                  isOnboarding={false}
                  onCancel={onHideBackup}
                  onSubmit={onBackupWallet}
                  recoveryPhrase={recoveryPhrase}
                />
              ) : (
                <CryptoView
                  onLockWallet={lockWallet}
                  needsBackup={!isWalletBackedUp}
                  onShowBackup={onShowBackup}
                  accounts={accounts}
                  onChangeTimeline={onChangeTimeline}
                  onSelectAsset={onSelectAsset}
                  portfolioBalance={fullPortfolioBalance}
                  selectedAsset={selectedAsset}
                  selectedUSDAssetPrice={selectedUSDAssetPrice}
                  selectedBTCAssetPrice={selectedBTCAssetPrice}
                  selectedAssetPriceHistory={formatedPriceHistory}
                  portfolioPriceHistory={portfolioPriceHistory}
                  selectedTimeline={selectedTimeline}
                  transactions={transactions}
                  userAssetList={userAssetList}
                  onConnectHardwareWallet={onConnectHardwareWallet}
                  onCreateAccount={onCreateAccount}
                  onImportAccount={onImportAccount}
                  isLoading={isFetchingPriceHistory}
                  showAddModal={showAddModal}
                  onToggleAddModal={onToggleAddModal}
                  onUpdateAccountName={onUpdateAccountName}
                  onUpdateWatchList={onUpdateWatchList}
                  userWatchList={['1']}
                />
              )}
            </>
          )}
        </>
      )
    }
  }, [
    isWalletCreated,
    isWalletLocked,
    recoveryPhrase,
    isWalletBackedUp,
    inputValue,
    hasIncorrectPassword,
    showRecoveryPhrase
  ])

  return (
    <WalletPageLayout>
      {/* <SideNav
        navList={NavOptions}
        selectedButton={view}
        onSubmit={navigateTo}
      /> */}
      <WalletSubViewLayout>
        {renderWallet}
        {/* {view === 'crypto' ? (
          renderWallet
        ) : (
          <div style={{ flex: 1, alignItems: 'center', justifyContent: 'center' }}>
            <h2>{view} view</h2>
          </div>
        )} */}
      </WalletSubViewLayout>
      {isWalletCreated && !isWalletLocked &&
        <WalletWidgetStandIn>
          <BuySendSwap
            accounts={accounts}
            orderType={orderType}
            swapToAsset={toAsset}
            swapFromAsset={fromAsset}
            selectedNetwork={selectedNetwork}
            selectedAccount={selectedAccount}
            exchangeRate={exchangeRate}
            sendAmount={sendAmount}
            fromAmount={fromAmount}
            fromAssetBalance='0'
            toAmount={toAmount}
            toAssetBalance='0'
            orderExpiration={orderExpiration}
            slippageTolerance={slippageTolerance}
            toAddress={toAddress}
            onSetToAddress={onSetToAddress}
            onSelectExpiration={onSelectExpiration}
            onSelectPresetAmount={onSelectPresetAmount}
            onSelectSlippageTolerance={onSelectSlippageTolerance}
            onSetExchangeRate={onSetExchangeRate}
            onSetSendAmount={onSetSendAmount}
            onSetFromAmount={onSetFromAmount}
            onSetToAmount={onSetToAmount}
            onSubmitSwap={onSubmitSwap}
            onSubmitSend={onSubmitSend}
            flipSwapAssets={flipSwapAssets}
            onSelectNetwork={onSelectNetwork}
            onSelectAccount={onSelectAccount}
            onToggleOrderType={onToggleOrderType}
            onSelectAsset={onSelectTransactAsset}

          />
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
