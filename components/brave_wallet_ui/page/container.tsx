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
  UserAssetInfoType,
  BraveWallet,
  BuySendSwapTypes,
  PageState,
  UpdateAccountNamePayloadType,
  WalletAccountType,
  WalletPageState,
  WalletRoutes,
  WalletState,
  SupportedTestNetworks
} from '../constants/types'
// import { NavOptions } from '../options/side-nav-options'
import BuySendSwap from '../stories/screens/buy-send-swap'
import Onboarding from '../stories/screens/onboarding'
import BackupWallet from '../stories/screens/backup-wallet'
import { AllNetworksOption } from '../options/network-filter-options'

// Utils
import Amount from '../utils/amount'
import { GetBuyOrFaucetUrl } from '../utils/buy-asset-url'
import { mojoTimeDeltaToJSDate } from '../../common/mojomUtils'
import { getTokensCoinType } from '../utils/network-utils'

import {
  findENSAddress,
  findUnstoppableDomainAddress,
  getBalance,
  getBlockchainTokenInfo,
  getChecksumEthAddress,
  isStrongPassword,
  onConnectHardwareWallet
} from '../common/async/lib'

// Hooks
import {
  useAssets,
  useBalance,
  usePreset,
  usePricing,
  useSend,
  useTokenInfo,
  useAssetManagement
} from '../common/hooks'
import { SweepstakesBanner } from '../components/desktop/sweepstakes-banner'

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
    portfolioPriceHistory,
    selectedPortfolioTimeline,
    isFetchingPortfolioPriceHistory,
    transactionSpotPrices,
    addUserAssetError,
    defaultWallet,
    isMetaMaskInstalled,
    defaultCurrencies,
    fullTokenList,
    userVisibleTokensInfo,
    selectedNetworkFilter
    userVisibleTokensInfo,
    coinMarketData,
    isLoadingCoinMarketData
  } = props.wallet

  // Page Props
  const {
    invalidMnemonic,
    mnemonic,
    selectedTimeline,
    selectedAsset,
    selectedAssetFiatPrice,
    selectedAssetCryptoPrice,
    selectedAssetPriceHistory,
    setupStillInProgress,
    isFetchingPriceHistory,
    privateKey,
    importAccountError,
    importWalletError,
    showAddModal,
    isCryptoWalletsInitialized,
    isMetaMaskInitialized
  } = props.page

  // const [view, setView] = React.useState<NavTypes>('crypto')
  const [inputValue, setInputValue] = React.useState<string>('')
  const [buyAmount, setBuyAmount] = React.useState('')
  const [selectedWidgetTab, setSelectedWidgetTab] = React.useState<BuySendSwapTypes>('buy')
  const [showVisibleAssetsModal, setShowVisibleAssetsModal] = React.useState<boolean>(false)
  const [sessionRoute, setSessionRoute] = React.useState<string | undefined>(undefined)

  const {
    sendAssetOptions,
    buyAssetOptions
  } = useAssets(
    selectedAccount,
    networkList,
    selectedNetwork,
    userVisibleTokensInfo,
    transactionSpotPrices
  )

  const {
    onFindTokenInfoByContractAddress,
    foundTokenInfoByContractAddress
  } = useTokenInfo(getBlockchainTokenInfo, userVisibleTokensInfo, fullTokenList, selectedNetwork)

  const {
    onSetSendAmount,
    onSetToAddressOrUrl,
    onSubmitSend,
    onSelectSendAsset,
    sendAmount,
    toAddressOrUrl,
    toAddress,
    addressError,
    addressWarning,
    selectedSendAsset,
    sendAmountValidationError
  } = useSend(
    findENSAddress,
    findUnstoppableDomainAddress,
    getChecksumEthAddress,
    sendAssetOptions,
    selectedAccount,
    props.walletActions.sendERC20Transfer,
    props.walletActions.sendTransaction,
    props.walletActions.sendERC721TransferFrom,
    props.wallet.fullTokenList
  )

  const {
    onAddCustomAsset,
    onUpdateVisibleAssets
  } = useAssetManagement()

  const { computeFiatAmount } = usePricing(transactionSpotPrices)
  const getAccountBalance = useBalance(networkList)
  const sendAssetBalance = getAccountBalance(selectedAccount, selectedSendAsset)

  const onSelectPresetAmount = usePreset(
    {
      onSetAmount: onSetSendAmount,
      asset: selectedSendAsset
    }
  )

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

  const onSelectNetwork = (network: BraveWallet.NetworkInfo) => {
    props.walletActions.selectNetwork(network)
  }

  const completeWalletSetup = (recoveryVerified: boolean) => {
    if (recoveryVerified) {
      props.walletPageActions.walletBackupComplete()
    }
    props.walletPageActions.walletSetupComplete()
  }

  const onBackupWallet = () => {
    props.walletPageActions.walletBackupComplete()
    history.goBack()
  }

  const restoreWallet = (mnemonic: string, password: string, isLegacy: boolean) => {
    props.walletPageActions.restoreWallet({ mnemonic, password, isLegacy })
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

  const restoreError = React.useMemo(() => {
    if (invalidMnemonic) {
      setTimeout(function () { props.walletPageActions.hasMnemonicError(false) }, 5000)
      return true
    }
    return false
  }, [invalidMnemonic])

  const recoveryPhrase = React.useMemo(() => {
    return (mnemonic || '').split(' ')
  }, [mnemonic])

  // This will scrape all the user's accounts and combine the asset balances for a single asset
  const fullAssetBalance = React.useCallback((asset: BraveWallet.BlockchainToken) => {
    const tokensCoinType = getTokensCoinType(networkList, asset)
    const amounts = accounts.filter((account) => account.coin === tokensCoinType).map((account) =>
      getAccountBalance(account, asset))

    // If a user has not yet created a FIL or SOL account,
    // we return 0 until they create an account
    if (amounts.length === 0) {
      return '0'
    }

    return amounts.reduce(function (a, b) {
      return a !== '' && b !== ''
        ? new Amount(a).plus(b).format()
        : ''
    })
  }, [accounts, networkList, getAccountBalance])

  // This looks at the users asset list and returns the full balance for each asset
  const userAssetList = React.useMemo(() => {
    const allAssets = userVisibleTokensInfo.map((asset) => ({
      asset: asset,
      assetBalance: fullAssetBalance(asset)
    }) as UserAssetInfoType)
    // By default we dont show any testnetwork assets
    if (selectedNetworkFilter.chainId === AllNetworksOption.chainId) {
      return allAssets.filter((asset) => !SupportedTestNetworks.includes(asset.asset.chainId))
    }
    // If chainId is Localhost we also do a check for coinType to return
    // the correct asset
    if (selectedNetworkFilter.chainId === BraveWallet.LOCALHOST_CHAIN_ID) {
      return allAssets.filter((asset) =>
        asset.asset.chainId === selectedNetworkFilter.chainId &&
        getTokensCoinType(networkList, asset.asset) === selectedNetworkFilter.coin
      )
    }
    // Filter by all other assets by chainId's
    return allAssets.filter((asset) => asset.asset.chainId === selectedNetworkFilter.chainId)
  }, [userVisibleTokensInfo, selectedNetworkFilter, fullAssetBalance, networkList])

  const onSelectAsset = (asset: BraveWallet.BlockchainToken) => {
    props.walletPageActions.selectAsset({ asset: asset, timeFrame: selectedTimeline })
  }

  // This will scrape all of the user's accounts and combine the fiat value for every asset
  const fullPortfolioBalance = React.useMemo(() => {
    const visibleAssetOptions = userAssetList
      .filter((token) =>
        token.asset.visible &&
        !token.asset.isErc721
      )

    if (visibleAssetOptions.length === 0) {
      return ''
    }

    const visibleAssetFiatBalances = visibleAssetOptions
      .map((item) => {
        return computeFiatAmount(item.assetBalance, item.asset.symbol, item.asset.decimals)
      })

    const grandTotal = visibleAssetFiatBalances.reduce(function (a, b) {
      return a.plus(b)
    })
    return grandTotal.formatAsFiat()
  }, [userAssetList, fullAssetBalance, computeFiatAmount])

  const onChangeTimeline = (timeline: BraveWallet.AssetPriceTimeframe) => {
    if (selectedAsset) {
      props.walletPageActions.selectAsset({ asset: selectedAsset, timeFrame: timeline })
    } else {
      props.walletActions.selectPortfolioTimeline(timeline)
    }
  }

  const formattedPriceHistory = React.useMemo(() => {
    return selectedAssetPriceHistory.map((obj) => {
      return {
        date: mojoTimeDeltaToJSDate(obj.date),
        close: Number(obj.price)
      }
    })
  }, [selectedAssetPriceHistory])

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

  const onSetImportWalletError = (hasError: boolean) => {
    props.walletPageActions.setImportWalletError({ hasError })
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

  const onImportCryptoWallets = (password: string, newPassword: string) => {
    props.walletPageActions.importFromCryptoWallets({ password, newPassword })
  }

  const onImportMetaMask = (password: string, newPassword: string) => {
    props.walletPageActions.importFromMetaMask({ password, newPassword })
  }

  const checkWalletsToImport = () => {
    props.walletPageActions.checkWalletsToImport()
  }

  const onOpenWalletSettings = () => {
    props.walletPageActions.openWalletSettings()
  }

  const onRetryTransaction = (transaction: BraveWallet.TransactionInfo) => {
    props.walletActions.retryTransaction(transaction)
  }

  const onSpeedupTransaction = (transaction: BraveWallet.TransactionInfo) => {
    props.walletActions.speedupTransaction(transaction)
  }

  const onCancelTransaction = (transaction: BraveWallet.TransactionInfo) => {
    props.walletActions.cancelTransaction(transaction)
  }

  const onAddNetwork = () => {
    props.walletActions.expandWalletNetworks()
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

  const onFetchCoinMarkets = (vsAsset: string, limit: number) => {
    props.walletActions.getCoinMarkets({
      vsAsset,
      limit
    })
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
      walletLocation !== WalletRoutes.Backup &&
      walletLocation !== WalletRoutes.Accounts &&
      walletLocation !== WalletRoutes.AddAccountModal &&
      walletLocation !== WalletRoutes.AddAssetModal &&
      walletLocation !== WalletRoutes.Market &&
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
                <OnboardingRestore
                  checkIsStrongPassword={isStrongPassword}
                  onRestore={restoreWallet}
                  toggleShowRestore={onToggleShowRestore}
                  hasRestoreError={restoreError}
                />
              </OnboardingWrapper>
            }
          </Route>
          <Route path={WalletRoutes.Onboarding} exact={true}>
            <Onboarding
              checkIsStrongPassword={isStrongPassword}
              recoveryPhrase={recoveryPhrase}
              onPasswordProvided={passwordProvided}
              onSubmit={completeWalletSetup}
              onShowRestore={onToggleShowRestore}
              isCryptoWalletsInitialized={isCryptoWalletsInitialized}
              isMetaMaskInitialized={isMetaMaskInitialized}
              importError={importWalletError}
              onSetImportError={onSetImportWalletError}
              onImportCryptoWallets={onImportCryptoWallets}
              onImportMetaMask={onImportMetaMask}
            />
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
                onChangeTimeline={onChangeTimeline}
                onSelectAsset={onSelectAsset}
                portfolioBalance={fullPortfolioBalance}
                selectedAsset={selectedAsset}
                selectedAssetFiatPrice={selectedAssetFiatPrice}
                selectedAssetCryptoPrice={selectedAssetCryptoPrice}
                selectedAssetPriceHistory={formattedPriceHistory}
                portfolioPriceHistory={portfolioPriceHistory}
                selectedTimeline={selectedTimeline}
                selectedPortfolioTimeline={selectedPortfolioTimeline}
                transactions={transactions}
                userAssetList={userAssetList}
                fullAssetList={fullTokenList}
                onConnectHardwareWallet={onConnectHardwareWallet}
                onCreateAccount={onCreateAccount}
                onImportAccount={onImportAccount}
                onImportFilecoinAccount={onImportFilecoinAccount}
                isFilecoinEnabled={isFilecoinEnabled}
                isSolanaEnabled={isSolanaEnabled}
                isLoading={isFetchingPriceHistory}
                showAddModal={showAddModal}
                onHideAddModal={onHideAddModal}
                onUpdateAccountName={onUpdateAccountName}
                selectedNetwork={selectedNetwork}
                isFetchingPortfolioPriceHistory={isFetchingPortfolioPriceHistory}
                onRemoveAccount={onRemoveAccount}
                privateKey={privateKey ?? ''}
                onDoneViewingPrivateKey={onDoneViewingPrivateKey}
                onViewPrivateKey={onViewPrivateKey}
                onImportAccountFromJson={onImportAccountFromJson}
                onSetImportError={onSetImportAccountError}
                hasImportError={importAccountError}
                onAddHardwareAccounts={onAddHardwareAccounts}
                transactionSpotPrices={transactionSpotPrices}
                userVisibleTokensInfo={userVisibleTokensInfo}
                getBalance={getBalance}
                onAddCustomAsset={onAddCustomAsset}
                addUserAssetError={addUserAssetError}
                defaultWallet={defaultWallet}
                onOpenWalletSettings={onOpenWalletSettings}
                onShowAddModal={onShowAddModal}
                isMetaMaskInstalled={isMetaMaskInstalled}
                onRetryTransaction={onRetryTransaction}
                onSpeedupTransaction={onSpeedupTransaction}
                onCancelTransaction={onCancelTransaction}
                onShowVisibleAssetsModal={onShowVisibleAssetsModal}
                showVisibleAssetsModal={showVisibleAssetsModal}
                onFindTokenInfoByContractAddress={onFindTokenInfoByContractAddress}
                foundTokenInfoByContractAddress={foundTokenInfoByContractAddress}
                onUpdateVisibleAssets={onUpdateVisibleAssets}
                isLoadingCoinMarketData={isLoadingCoinMarketData}
                coinMarkets={coinMarketData}
                onFetchCoinMarkets={onFetchCoinMarkets}
                tradableAssets={swapAssetOptions}
              />
            }
          </Route>
        </WalletSubViewLayout>
      </Switch>
      {hideMainComponents &&
        <WalletWidgetStandIn>
          <BuySendSwap
            networkList={networkList}
            selectedNetwork={selectedNetwork}
            selectedTab={selectedWidgetTab}
            buyAmount={buyAmount}
            sendAmount={sendAmount}
            addressError={addressError}
            addressWarning={addressWarning}
            sendAmountValidationError={sendAmountValidationError}
            toAddressOrUrl={toAddressOrUrl}
            toAddress={toAddress}
            buyAssetOptions={buyAssetOptions}
            selectedSendAsset={selectedSendAsset}
            sendAssetBalance={sendAssetBalance}
            sendAssetOptions={sendAssetOptions}
            defaultCurrencies={defaultCurrencies}
            onSetBuyAmount={onSetBuyAmount}
            onSetToAddressOrUrl={onSetToAddressOrUrl}
            onSelectPresetSendAmount={onSelectPresetAmount}
            onSetSendAmount={onSetSendAmount}
            onSubmitSend={onSubmitSend}
            onSubmitBuy={onSubmitBuy}
            onSelectNetwork={onSelectNetwork}
            onSelectAccount={onSelectAccount}
            onSelectTab={setSelectedWidgetTab}
            onSelectSendAsset={onSelectSendAsset}
            onAddNetwork={onAddNetwork}
            onAddAsset={onShowVisibleAssetsModal}
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
