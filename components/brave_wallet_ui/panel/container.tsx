// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { connect } from 'react-redux'
import { bindActionCreators, Dispatch } from 'redux'
import {
  ConnectWithSite,
  ConnectedPanel,
  Panel,
  WelcomePanel,
  SignPanel,
  AllowSpendPanel,
  AllowAddNetworkPanel,
  ConfirmTransactionPanel
} from '../components/extension'
import {
  Send,
  Buy,
  SelectAsset,
  SelectAccount,
  SelectNetwork
} from '../components/buy-send-swap/'
import { AppList } from '../components/shared'
import { filterAppList } from '../utils/filter-app-list'
import {
  ScrollContainer,
  StyledExtensionWrapper,
  SelectContainer,
  SignContainer
} from '../stories/style'
import { SendWrapper } from './style'
import store from './store'
import * as WalletPanelActions from './actions/wallet_panel_actions'
import * as WalletActions from '../common/actions/wallet_actions'
import {
  AppObjectType,
  AppsListType,
  WalletState,
  PanelState,
  PanelTypes,
  WalletPanelState,
  WalletAccountType,
  BuySendSwapViewTypes,
  AssetOptionType,
  Network
} from '../constants/types'
import { AppsList } from '../options/apps-list-options'
import LockPanel from '../components/extension/lock-panel'
import { AssetOptions } from '../options/asset-options'
import { WyreAssetOptions } from '../options/wyre-asset-options'
import { NetworkOptions } from '../options/network-options'
import { BuyAssetUrl } from '../utils/buy-asset-url'

import { formatBalance, toWei } from '../utils/format-balances'

type Props = {
  panel: PanelState
  wallet: WalletState
  walletPanelActions: typeof WalletPanelActions
  walletActions: typeof WalletActions
}

function mapStateToProps (state: WalletPanelState): Partial<Props> {
  return {
    panel: state.panel,
    wallet: state.wallet
  }
}

function mapDispatchToProps (dispatch: Dispatch): Partial<Props> {
  return {
    walletPanelActions: bindActionCreators(WalletPanelActions, store.dispatch.bind(store)),
    walletActions: bindActionCreators(WalletActions, store.dispatch.bind(store))
  }
}

function Container (props: Props) {
  const {
    accounts,
    selectedAccount,
    selectedNetwork,
    isWalletLocked,
    favoriteApps,
    hasIncorrectPassword,
    hasInitialized,
    userVisibleTokensInfo,
    isWalletCreated
  } = props.wallet

  const {
    connectedSiteOrigin,
    panelTitle,
    selectedPanel,
    showSignTransaction,
    showAllowSpendERC20Token,
    showAllowAddNetwork,
    showConfirmTransaction
  } = props.panel

  // TODO(petemill): If initial data or UI takes a noticeable amount of time to arrive
  // consider rendering a "loading" indicator when `hasInitialized === false`, and
  // also using `React.lazy` to put all the main UI in a separate JS bundle and display
  // that loading indicator ASAP.
  const [selectedAccounts, setSelectedAccounts] = React.useState<WalletAccountType[]>([])
  const [filteredAppsList, setFilteredAppsList] = React.useState<AppsListType[]>(AppsList)
  const [walletConnected, setWalletConnected] = React.useState<boolean>(true)
  const [selectedAsset, setSelectedAsset] = React.useState<AssetOptionType>(AssetOptions[0])
  const [selectedWyreAsset, setSelectedWyreAsset] = React.useState<AssetOptionType>(WyreAssetOptions[0])
  const [showSelectAsset, setShowSelectAsset] = React.useState<boolean>(false)
  const [toAddress, setToAddress] = React.useState('')
  const [sendAmount, setSendAmount] = React.useState('')
  const [buyAmount, setBuyAmount] = React.useState('')

  const onSetBuyAmount = (value: string) => {
    setBuyAmount(value)
  }

  const onSubmitBuy = () => {
    const url = BuyAssetUrl(selectedNetwork, selectedWyreAsset, selectedAccount, buyAmount)
    if (url) {
      chrome.tabs.create({ url: url }, () => {
        if (chrome.runtime.lastError) {
          console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
        }
      })
    }
  }

  const onChangeSendView = (view: BuySendSwapViewTypes) => {
    if (view === 'assets') {
      setShowSelectAsset(true)
    }
  }

  const onHideSelectAsset = () => {
    setShowSelectAsset(false)
  }

  const onSelectAsset = (asset: AssetOptionType) => () => {
    if (selectedPanel === 'buy') {
      setSelectedWyreAsset(asset)
    } else {
      setSelectedAsset(asset)
    }
    setShowSelectAsset(false)
  }

  const onInputChange = (value: string, name: string) => {
    if (name === 'address') {
      setToAddress(value)
    } else {
      setSendAmount(value)
    }
  }

  const selectedAssetBalance = React.useMemo(() => {
    if (!selectedAccount || !selectedAccount.tokens) {
      return '0'
    }
    const token = selectedAccount.tokens.find((token) => token.asset.symbol === selectedAsset.symbol)
    return token ? formatBalance(token.assetBalance, token.asset.decimals) : '0'
  }, [accounts, selectedAccount, selectedAsset])

  const onSelectPresetAmount = (percent: number) => {
    const amount = Number(selectedAssetBalance) * percent
    setSendAmount(amount.toString())
  }

  const onSubmitSend = () => {
    const asset = userVisibleTokensInfo.find((asset) => asset.symbol === selectedAsset.symbol)
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

  const toggleConnected = () => {
    setWalletConnected(!walletConnected)
  }

  const [readyToConnect, setReadyToConnect] = React.useState<boolean>(false)
  const selectAccount = (account: WalletAccountType) => {
    const newList = [...selectedAccounts, account]
    setSelectedAccounts(newList)
  }
  const removeAccount = (account: WalletAccountType) => {
    const newList = selectedAccounts.filter(
      (accounts) => accounts.id !== account.id
    )
    setSelectedAccounts(newList)
  }
  const [inputValue, setInputValue] = React.useState<string>('')
  const onSubmit = () => {
    props.walletPanelActions.connectToSite({
      selectedAccounts,
      siteToConnectTo: connectedSiteOrigin
    })
  }
  const primaryAction = () => {
    if (!readyToConnect) {
      setReadyToConnect(true)
    } else {
      onSubmit()
      setSelectedAccounts([])
      setReadyToConnect(false)
    }
  }
  const secondaryAction = () => {
    if (readyToConnect) {
      setReadyToConnect(false)
    } else {
      props.walletPanelActions.cancelConnectToSite({
        selectedAccounts,
        siteToConnectTo: props.panel.connectedSiteOrigin
      })
      setSelectedAccounts([])
      setReadyToConnect(false)
    }
  }
  const unlockWallet = () => {
    props.walletActions.unlockWallet({ password: inputValue })
    setInputValue('')
  }
  const onLockWallet = () => {
    props.walletActions.lockWallet()
  }
  const handlePasswordChanged = (value: string) => {
    setInputValue(value)
    if (hasIncorrectPassword) {
      props.walletActions.hasIncorrectPassword(false)
    }
  }
  const onRestore = () => {
    props.walletPanelActions.restoreWallet()
  }
  const onSetup = () => {
    props.walletPanelActions.setupWallet()
  }
  const addToFavorites = (app: AppObjectType) => {
    props.walletActions.addFavoriteApp(app)
  }

  const navigateTo = (path: PanelTypes) => {
    if (path === 'expanded') {
      props.walletPanelActions.expandWallet()
    } else {
      props.walletPanelActions.navigateTo(path)
    }
  }

  const browseMore = () => {
    props.walletPanelActions.openWalletApps()
  }

  const removeFromFavorites = (app: AppObjectType) => {
    props.walletActions.removeFavoriteApp(app)
  }

  const filterList = (event: any) => {
    filterAppList(event, AppsList, setFilteredAppsList)
  }

  const onSelectAccount = (account: WalletAccountType) => () => {
    props.walletActions.selectAccount(account)
    props.walletPanelActions.navigateTo('main')
  }

  const onSelectNetwork = (network: Network) => () => {
    props.walletActions.selectNetwork(network)
    props.walletPanelActions.navigateTo('main')
  }

  const onReturnToMain = () => {
    props.walletPanelActions.navigateTo('main')
  }

  const onShowMoreModal = () => {
    // Need to build out Show More Modal for Wallet Panels
  }

  const onCancelSigning = () => {
    // Logic here to cancel signing
  }

  const onSignTransaction = () => {
    // Logic here to sign a transaction
  }

  const onRejectERC20Spend = () => {
    // Logic here to Reject an ERC20 Spend Transactiong
  }

  const onConfirmERC20Spend = () => {
    // Logic here to Confirm an ERC20 Spend Transaction
  }

  const onApproveAddNetwork = () => {
    // Logic Here to Approve Adding a Network
  }

  const onCancelAddNetwork = () => {
    // Logic Here to Cancel Adding a Network
  }

  const onRejectTransaction = () => {
    // Logic here to Reject a Transaction
  }

  const onConfirmTransaction = () => {
    // Logic here to Confirm a Transaction
  }

  const onOpenSettings = () => {
    props.walletPanelActions.openWalletSettings()
  }

  // Example of a ERC20 Spend Payload to be passed to the
  // Allow Spend Panel
  const ERC20SpendPayloadExample = {
    siteUrl: 'https://app.compound.finance',
    contractAddress: '0x3f29A1da97149722eB09c526E4eAd698895b426',
    erc20Token: {
      contractAddress: '0x0d8775f648430679a709e98d2b0cb6250d2887ef',
      name: 'Basic Attention Token',
      isErc20: true,
      isErc721: false,
      symbol: 'BAT',
      decimals: 18,
      icon: ''
    },
    transactionFeeWei: '0.002447',
    transactionFeeFiat: '$6.57'
  }

  // Example of a Add Network Payload to be passed to the
  // Allow Add Network Panel
  const networkPayloadExample = {
    siteUrl: 'https://app.compound.finance',
    contractAddress: '0x3f29A1da97149722eB09c526E4eAd698895b426',
    chainInfo: {
      chainId: '',
      name: 'BSC (Binance Smart Chain)',
      url: 'https://bsc.binance.com'
    }
  }

  // Example of a Confirm Transaction Payload to be passed to the
  // Confirm Transaction Panel
  const transactionPayloadExample = {
    transactionAmount: '68000000000000000000',
    transactionGas: '7548000000000000',
    toAddress: '0x0d8775f648430679a709e98d2b0cb6250d2887ef',
    erc20Token: {
      contractAddress: '0x0d8775f648430679a709e98d2b0cb6250d2887ef',
      name: 'Basic Attention Token',
      isErc20: true,
      isErc721: false,
      symbol: 'BAT',
      decimals: 18,
      icon: ''
    },
    tokenPrice: '0.35',
    ethPrice: '3058.35'
  }

  if (!hasInitialized || !accounts) {
    return null
  }

  if (!isWalletCreated) {
    return (
      <StyledExtensionWrapper>
        <WelcomePanel onRestore={onRestore} onSetup={onSetup} />
      </StyledExtensionWrapper>)
  }

  if (isWalletLocked) {
    return (
      <StyledExtensionWrapper>
        <LockPanel
          hasPasswordError={hasIncorrectPassword}
          onSubmit={unlockWallet}
          disabled={inputValue === ''}
          onPasswordChanged={handlePasswordChanged}
        />
      </StyledExtensionWrapper>
    )
  }

  if (showConfirmTransaction) {
    return (
      <SignContainer>
        <ConfirmTransactionPanel
          onConfirm={onConfirmTransaction}
          onReject={onRejectTransaction}
          onClickMore={onShowMoreModal}
          selectedAccount={selectedAccount}
          selectedNetwork={selectedNetwork}
          transactionPayload={transactionPayloadExample}
        />
      </SignContainer>
    )
  }

  if (showAllowAddNetwork) {
    return (
      <SignContainer>
        <AllowAddNetworkPanel
          onApprove={onApproveAddNetwork}
          onCancel={onCancelAddNetwork}
          networkPayload={networkPayloadExample}
          selectedNetwork={selectedNetwork}
        />
      </SignContainer>
    )
  }

  if (showSignTransaction) {
    return (
      <SignContainer>
        <SignPanel
          message='Pass Sign Transaction Message Here'
          onCancel={onCancelSigning}
          onClickMore={onShowMoreModal}
          onSign={onSignTransaction}
          selectedAccount={selectedAccount}
          selectedNetwork={selectedNetwork}
        />
      </SignContainer>
    )
  }

  if (showAllowSpendERC20Token) {
    return (
      <SignContainer>
        <AllowSpendPanel
          onReject={onRejectERC20Spend}
          onConfirm={onConfirmERC20Spend}
          selectedNetwork={selectedNetwork}
          spendPayload={ERC20SpendPayloadExample}
        />
      </SignContainer>
    )
  }

  if (showSelectAsset) {
    return (
      <SelectContainer>
        <SelectAsset
          assets={selectedPanel === 'buy' ? WyreAssetOptions : AssetOptions}
          onSelectAsset={onSelectAsset}
          onBack={onHideSelectAsset}
        />
      </SelectContainer>
    )
  }

  if (selectedPanel === 'networks') {
    return (
      <SelectContainer>
        <SelectNetwork
          networks={NetworkOptions}
          onBack={onReturnToMain}
          onSelectNetwork={onSelectNetwork}
        />
      </SelectContainer>
    )
  }

  if (selectedPanel === 'accounts') {
    return (
      <SelectContainer>
        <SelectAccount
          accounts={accounts}
          onBack={onReturnToMain}
          onSelectAccount={onSelectAccount}
        />
      </SelectContainer>
    )
  }

  if (selectedPanel === 'apps') {
    return (
      <StyledExtensionWrapper>
        <Panel
          navAction={navigateTo}
          title={panelTitle}
          useSearch={selectedPanel === 'apps'}
          searchAction={selectedPanel === 'apps' ? filterList : undefined}
        >
          <ScrollContainer>
            <AppList
              list={filteredAppsList}
              favApps={favoriteApps}
              addToFav={addToFavorites}
              removeFromFav={removeFromFavorites}
              action={browseMore}
            />
          </ScrollContainer>
        </Panel>
      </StyledExtensionWrapper>)
  }

  if (selectedPanel === 'connectWithSite') {
    const accountsToConnect = props.wallet.accounts.filter(
      (account) => props.panel.connectingAccounts.includes(account.address.toLowerCase())
    )
    return (
      <StyledExtensionWrapper>
        <ConnectWithSite
          siteURL={connectedSiteOrigin}
          isReady={readyToConnect}
          accounts={accountsToConnect}
          primaryAction={primaryAction}
          secondaryAction={secondaryAction}
          selectAccount={selectAccount}
          removeAccount={removeAccount}
          selectedAccounts={selectedAccounts}
        />
      </StyledExtensionWrapper>)
  }

  if (selectedPanel === 'send') {
    return (
      <StyledExtensionWrapper>
        <Panel
          navAction={navigateTo}
          title={panelTitle}
          useSearch={false}
        >
          <SendWrapper>
            <Send
              onChangeSendView={onChangeSendView}
              onInputChange={onInputChange}
              onSelectPresetAmount={onSelectPresetAmount}
              onSubmit={onSubmitSend}
              selectedAsset={selectedAsset}
              selectedAssetAmount={sendAmount}
              selectedAssetBalance={selectedAssetBalance}
              toAddress={toAddress}
            />
          </SendWrapper>
        </Panel>
      </StyledExtensionWrapper>)
  }

  if (selectedPanel === 'buy') {
    return (
      <StyledExtensionWrapper>
        <Panel
          navAction={navigateTo}
          title={panelTitle}
          useSearch={false}
        >
          <SendWrapper>
            <Buy
              onChangeBuyView={onChangeSendView}
              onInputChange={onSetBuyAmount}
              onSubmit={onSubmitBuy}
              selectedAsset={selectedWyreAsset}
              buyAmount={buyAmount}
              selectedNetwork={selectedNetwork}
            />
          </SendWrapper>
        </Panel>
      </StyledExtensionWrapper>)
  }

  return (
    <>
      <ConnectedPanel
        selectedAccount={selectedAccount}
        selectedNetwork={selectedNetwork}
        isConnected={walletConnected}
        connectAction={toggleConnected}
        navAction={navigateTo}
        onLockWallet={onLockWallet}
        onOpenSettings={onOpenSettings}
      />
    </>)

}

export default connect(mapStateToProps, mapDispatchToProps)(Container)
