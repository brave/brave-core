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
  LockScreen,
  OnboardingRestore
} from '../components/desktop'
import {
  // NavTypes,
  WalletState,
  PageState,
  WalletPageState,
  AssetPriceTimeframe,
  AccountAssetOptionType,
  OrderTypes,
  SlippagePresetObjectType,
  ExpirationPresetObjectType,
  ToOrFromType,
  WalletAccountType,
  TokenInfo,
  UpdateAccountNamePayloadType,
  EthereumChain
} from '../constants/types'
// import { NavOptions } from '../options/side-nav-options'
import BuySendSwap from '../stories/screens/buy-send-swap'
import Onboarding from '../stories/screens/onboarding'
import BackupWallet from '../stories/screens/backup-wallet'
import { formatPrices } from '../utils/format-prices'
import { BuyAssetUrl } from '../utils/buy-asset-url'
import { convertMojoTimeToJS } from '../utils/mojo-time'
import { AssetOptions, AccountAssetOptions } from '../options/asset-options'
import { WyreAccountAssetOptions } from '../options/wyre-asset-options'
import { SlippagePresetOptions } from '../options/slippage-preset-options'
import { ExpirationPresetOptions } from '../options/expiration-preset-options'
import {
  HardwareWalletAccount,
  HardwareWalletConnectOpts
} from '../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import * as Result from '../common/types/result'

import { formatBalance, toWei } from '../utils/format-balances'

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
    networkList,
    transactions,
    selectedNetwork,
    selectedAccount,
    hasInitialized,
    userVisibleTokens,
    userVisibleTokensInfo,
    fullTokenList,
    portfolioPriceHistory,
    selectedPortfolioTimeline,
    isFetchingPortfolioPriceHistory
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
    setupStillInProgress,
    isFetchingPriceHistory,
    showIsRestoring,
    privateKey
  } = props.page

  // const [view, setView] = React.useState<NavTypes>('crypto')
  const [inputValue, setInputValue] = React.useState<string>('')
  const [showAddModal, setShowAddModal] = React.useState<boolean>(false)
  const [exchangeRate, setExchangeRate] = React.useState('')
  const [toAddress, setToAddress] = React.useState('')
  const [buyAmount, setBuyAmount] = React.useState('')
  const [sendAmount, setSendAmount] = React.useState('')
  const [fromAmount, setFromAmount] = React.useState('')
  const [toAmount, setToAmount] = React.useState('')
  const [slippageTolerance, setSlippageTolerance] = React.useState<SlippagePresetObjectType>(SlippagePresetOptions[0])
  const [orderExpiration, setOrderExpiration] = React.useState<ExpirationPresetObjectType>(ExpirationPresetOptions[0])
  const [orderType, setOrderType] = React.useState<OrderTypes>('market')
  // const [showRestore, setShowRestore] = React.useState<boolean>(false)

  // TODO (DOUGLAS): This needs to be set up in the Reducer in a future PR
  const [fromAsset, setFromAsset] = React.useState<AccountAssetOptionType>(AccountAssetOptions[0])
  const [toAsset, setToAsset] = React.useState<AccountAssetOptionType>(AccountAssetOptions[1])
  const onSelectTransactAsset = (asset: AccountAssetOptionType, toOrFrom: ToOrFromType) => {
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

  const onToggleShowRestore = () => {
    props.walletPageActions.setShowIsRestoring(!showIsRestoring)
  }

  const onSetToAddress = (value: string) => {
    setToAddress(value)
  }

  const onSetBuyAmount = (value: string) => {
    setBuyAmount(value)
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

  const onToggleOrderType = () => {
    if (orderType === 'market') {
      setOrderType('limit')
    } else {
      setOrderType('market')
    }
  }

  const onSelectAccount = (account: WalletAccountType) => {
    props.walletActions.selectAccount(account)
  }

  const onSelectNetwork = (network: EthereumChain) => {
    props.walletActions.selectNetwork(network)
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

  const restoreWallet = (mnemonic: string, password: string, isLegacy: boolean) => {
    // isLegacy prop will be passed into the restoreWallet action once the keyring is setup handle the derivation.
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

  // This will scrape all of the user's accounts and combine the asset balances for a single asset
  const fullAssetBalance = (asset: TokenInfo) => {
    const amounts = accounts.map((account) => {
      let balance = 0
      const found = account.tokens.find((token) => token.asset.contractAddress === asset.contractAddress)
      if (found) {
        balance = Number(formatBalance(found.assetBalance, found.asset.decimals))
      }
      return balance
    })
    const grandTotal = amounts.reduce(function (a, b) {
      return a + b
    }, 0)
    return grandTotal
  }

  // This will scrape all of the user's accounts and combine the fiat value for a single asset
  const fullAssetFiatBalance = (asset: TokenInfo) => {
    const amounts = accounts.map((account) => {
      let fiatBalance = 0
      const found = account.tokens.find((token) => token.asset.contractAddress === asset.contractAddress)
      if (found) {
        fiatBalance = Number(found.fiatBalance)
      }
      return fiatBalance
    })
    const grandTotal = amounts.reduce(function (a, b) {
      return a + b
    }, 0)
    return grandTotal
  }

  // This looks at the users asset list and returns the full balance for each asset
  const userAssetList = React.useMemo(() => {
    const newListWithIcon = userVisibleTokensInfo.map((asset) => {
      const icon = AssetOptions.find((a) => asset.symbol === a.symbol)?.icon
      return { ...asset, icon: icon }
    })
    return newListWithIcon.map((asset) => {
      return {
        asset: asset,
        assetBalance: fullAssetBalance(asset).toString(),
        fiatBalance: fullAssetFiatBalance(asset).toString()
      }
    })
  }, [userVisibleTokensInfo, accounts])

  const onSelectAsset = (asset: TokenInfo) => {
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
    return formatPrices(grandTotal)
  }, [userAssetList])

  const onChangeTimeline = (timeline: AssetPriceTimeframe) => {
    if (selectedAsset) {
      props.walletPageActions.selectAsset({ asset: selectedAsset, timeFrame: timeline })
    } else {
      if (parseFloat(fullPortfolioBalance) !== 0) {
        props.walletActions.selectPortfolioTimeline(timeline)
      }
    }
  }

  const formatedPriceHistory = React.useMemo(() => {
    const formated = selectedAssetPriceHistory.map((obj) => {
      return {
        date: convertMojoTimeToJS(obj.date),
        close: Number(obj.price)
      }
    })
    return formated
  }, [selectedAssetPriceHistory])

  const fromAssetBalance = React.useMemo(() => {
    if (!selectedAccount) {
      return '0'
    }
    const token = selectedAccount.tokens.find((token) => token.asset.symbol === fromAsset.asset.symbol)
    return token ? formatBalance(token.assetBalance, token.asset.decimals) : '0'
  }, [accounts, selectedAccount, fromAsset])

  const onSelectPresetFromAmount = (percent: number) => {
    const asset = userVisibleTokensInfo.find((asset) => asset.symbol === fromAsset.asset.symbol)
    const amount = Number(fromAsset.assetBalance) * percent
    const formatedAmmount = formatBalance(amount.toString(), asset?.decimals ?? 18)
    setFromAmount(formatedAmmount)
  }

  const onSelectPresetSendAmount = (percent: number) => {
    const asset = userVisibleTokensInfo.find((asset) => asset.symbol === fromAsset.asset.symbol)
    const amount = Number(fromAsset.assetBalance) * percent
    const formatedAmmount = formatBalance(amount.toString(), asset?.decimals ?? 18)
    setSendAmount(formatedAmmount)
  }

  const onToggleAddModal = () => {
    setShowAddModal(!showAddModal)
  }

  const onCreateAccount = (name: string) => {
    const created = props.walletPageActions.addAccount({ accountName: name })
    if (created) {
      onToggleAddModal()
    }
  }

  const onSubmitBuy = (asset: AccountAssetOptionType) => {
    const url = BuyAssetUrl(selectedNetwork.chainId, asset, selectedAccount, buyAmount)
    if (url) {
      window.open(url, '_blank')
    }
  }

  const onConnectHardwareWallet = (opts: HardwareWalletConnectOpts): Result.Type<HardwareWalletAccount[]> => {
    // TODO (DOUGLAS): Add logic to connect a hardware wallet

    return []
  }

  const onImportAccount = (accountName: string, privateKey: string) => {
    const imported = props.walletPageActions.importAccount({ accountName, privateKey })
    if (imported) {
      onToggleAddModal()
    }
  }

  const onRemoveAccount = (address: string) => {
    props.walletPageActions.removeImportedAccount({ address })
  }

  const onUpdateAccountName = (payload: UpdateAccountNamePayloadType): { success: boolean } => {
    const result = props.walletPageActions.updateAccountName(payload)
    return result ? { success: true } : { success: false }
  }

  const onUpdateVisibleTokens = (visibleTokens: string[]) => {
    props.walletActions.updateVisibleTokens(visibleTokens)
  }

  const onSubmitSwap = () => {
    // TODO (DOUGLAS): logic Here to submit a swap transaction
  }

  const onSubmitSend = () => {
    const asset = userVisibleTokensInfo.find((asset) => asset.symbol === fromAsset.asset.symbol)
    // TODO: Use real gas price & limit
    props.walletActions.sendTransaction({
      from: selectedAccount.address,
      to: toAddress,
      value: toWei(sendAmount, asset?.decimals ?? 0),
      contractAddress: asset?.contractAddress ?? '',
      gasPrice: '0x20000000000',
      gasLimit: '0xFDE8'
    })
  }

  const fetchFullTokenList = () => {
    props.walletActions.getAllTokensList()
  }

  const onViewPrivateKey = (address: string, isDefault: boolean) => {
    props.walletPageActions.viewPrivateKey({ address, isDefault })
  }

  const onDoneViewingPrivateKey = () => {
    props.walletPageActions.doneViewingPrivateKey()
  }

  const renderWallet = React.useMemo(() => {
    if (!hasInitialized) {
      return null
    }
    if (showIsRestoring) {
      return (
        <OnboardingRestore
          onRestore={restoreWallet}
          toggleShowRestore={onToggleShowRestore}
          hasRestoreError={restorError}
        />
      )
    }
    if (!isWalletCreated || setupStillInProgress) {
      return (
        <Onboarding
          recoveryPhrase={recoveryPhrase}
          onPasswordProvided={passwordProvided}
          onSubmit={completeWalletSetup}
          onShowRestore={onToggleShowRestore}
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
              onShowRestore={onToggleShowRestore}
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
                  networkList={networkList}
                  onChangeTimeline={onChangeTimeline}
                  onSelectAsset={onSelectAsset}
                  portfolioBalance={fullPortfolioBalance}
                  selectedAsset={selectedAsset}
                  selectedUSDAssetPrice={selectedUSDAssetPrice}
                  selectedBTCAssetPrice={selectedBTCAssetPrice}
                  selectedAssetPriceHistory={formatedPriceHistory}
                  portfolioPriceHistory={portfolioPriceHistory}
                  selectedTimeline={selectedTimeline}
                  selectedPortfolioTimeline={selectedPortfolioTimeline}
                  transactions={transactions}
                  userAssetList={userAssetList}
                  fullAssetList={fullTokenList}
                  onConnectHardwareWallet={onConnectHardwareWallet}
                  onCreateAccount={onCreateAccount}
                  onImportAccount={onImportAccount}
                  isLoading={isFetchingPriceHistory}
                  showAddModal={showAddModal}
                  onToggleAddModal={onToggleAddModal}
                  onUpdateAccountName={onUpdateAccountName}
                  onUpdateVisibleTokens={onUpdateVisibleTokens}
                  fetchFullTokenList={fetchFullTokenList}
                  userWatchList={userVisibleTokens}
                  selectedNetwork={selectedNetwork}
                  onSelectNetwork={onSelectNetwork}
                  isFetchingPortfolioPriceHistory={isFetchingPortfolioPriceHistory}
                  onRemoveAccount={onRemoveAccount}
                  privateKey={privateKey ?? ''}
                  onDoneViewingPrivateKey={onDoneViewingPrivateKey}
                  onViewPrivateKey={onViewPrivateKey}
                />
              )}
            </>
          )}
        </>
      )
    }
  }, [
    privateKey,
    fullTokenList,
    isWalletCreated,
    isWalletLocked,
    recoveryPhrase,
    isWalletBackedUp,
    inputValue,
    hasIncorrectPassword,
    showRecoveryPhrase,
    userAssetList,
    userVisibleTokens,
    userVisibleTokensInfo,
    portfolioPriceHistory,
    isFetchingPortfolioPriceHistory
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
      {(isWalletCreated && !setupStillInProgress) && !isWalletLocked &&
        <WalletWidgetStandIn>
          <BuySendSwap
            accounts={accounts}
            networkList={networkList}
            orderType={orderType}
            swapToAsset={toAsset}
            swapFromAsset={fromAsset}
            selectedNetwork={selectedNetwork}
            selectedAccount={selectedAccount}
            exchangeRate={exchangeRate}
            buyAmount={buyAmount}
            sendAmount={sendAmount}
            fromAmount={fromAmount}
            fromAssetBalance={fromAssetBalance}
            toAmount={toAmount}
            toAssetBalance='0'
            orderExpiration={orderExpiration}
            slippageTolerance={slippageTolerance}
            toAddress={toAddress}
            buyAssetOptions={WyreAccountAssetOptions}
            sendAssetOptions={selectedAccount.tokens}
            swapAssetOptions={AccountAssetOptions}
            onSetBuyAmount={onSetBuyAmount}
            onSetToAddress={onSetToAddress}
            onSelectExpiration={onSelectExpiration}
            onSelectPresetFromAmount={onSelectPresetFromAmount}
            onSelectPresetSendAmount={onSelectPresetSendAmount}
            onSelectSlippageTolerance={onSelectSlippageTolerance}
            onSetExchangeRate={onSetExchangeRate}
            onSetSendAmount={onSetSendAmount}
            onSetFromAmount={onSetFromAmount}
            onSetToAmount={onSetToAmount}
            onSubmitSwap={onSubmitSwap}
            onSubmitSend={onSubmitSend}
            onSubmitBuy={onSubmitBuy}
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
