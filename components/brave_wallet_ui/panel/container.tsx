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
  AllowAddChangeNetworkPanel,
  ConfirmTransactionPanel,
  ConnectHardwareWalletPanel,
  SitePermissions,
  AddSuggestedTokenPanel,
  TransactionsPanel,
  TransactionDetailPanel,
  AssetsPanel,
  EncryptionKeyPanel
} from '../components/extension'
import {
  Send,
  Buy,
  SelectAsset,
  SelectAccount,
  SelectNetworkWithHeader,
  Swap
} from '../components/buy-send-swap/'
import { AppList } from '../components/shared'
import { filterAppList } from '../utils/filter-app-list'
import {
  ScrollContainer,
  StyledExtensionWrapper,
  SelectContainer,
  LongWrapper,
  ConnectWithSiteWrapper
} from '../stories/style'
import {
  SendWrapper,
  PanelWrapper,
  WelcomePanelWrapper
} from './style'
import store from './store'
import * as WalletPanelActions from './actions/wallet_panel_actions'
import * as WalletActions from '../common/actions/wallet_actions'
import {
  AppsListType,
  BraveWallet,
  Origin,
  WalletState,
  PanelState,
  PanelTypes,
  WalletPanelState,
  WalletAccountType,
  BuySendSwapViewTypes,
  ToOrFromType,
  WalletOrigin
} from '../constants/types'
import { AppsList } from '../options/apps-list-options'
import LockPanel from '../components/extension/lock-panel'
import { GetBuyOrFaucetUrl } from '../utils/buy-asset-url'
import { getNetworkInfo } from '../utils/network-utils'
import { isHardwareAccount } from '../utils/address-utils'
import { useAssets, useSwap, useSend } from '../common/hooks'

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
    selectedPendingTransaction,
    isWalletLocked,
    favoriteApps,
    hasIncorrectPassword,
    hasInitialized,
    isWalletCreated,
    networkList,
    transactionSpotPrices,
    connectedAccounts,
    activeOrigin,
    defaultCurrencies,
    transactions,
    userVisibleTokensInfo,
    defaultAccounts
  } = props.wallet

  const {
    connectToSiteOrigin,
    panelTitle,
    selectedPanel,
    addChainRequest,
    signMessageData,
    switchChainRequest,
    suggestedTokenRequest,
    getEncryptionPublicKeyRequest,
    decryptRequest,
    connectingAccounts,
    selectedTransaction
  } = props.panel

  // TODO(petemill): If initial data or UI takes a noticeable amount of time to arrive
  // consider rendering a "loading" indicator when `hasInitialized === false`, and
  // also using `React.lazy` to put all the main UI in a separate JS bundle and display
  // that loading indicator ASAP.
  const [selectedAccounts, setSelectedAccounts] = React.useState<WalletAccountType[]>([])
  const [filteredAppsList, setFilteredAppsList] = React.useState<AppsListType[]>(AppsList)
  const [showSelectAsset, setShowSelectAsset] = React.useState<boolean>(false)
  const [buyAmount, setBuyAmount] = React.useState('')

  const {
    sendAssetOptions,
    buyAssetOptions,
    panelUserAssetList
  } = useAssets()

  const [selectedWyreAsset, setSelectedWyreAsset] = React.useState<BraveWallet.BlockchainToken>(buyAssetOptions[0])

  const swap = useSwap()
  const {
    filteredAssetList,
    isSwapSupported,
    setSwapToOrFrom,
    swapToOrFrom,
    onSelectTransactAsset
  } = swap

  const {
    selectSendAsset: onSelectSendAsset
  } = useSend()

  React.useEffect(() => {
    if (selectedPanel === 'connectWithSite') {
      const foundDefaultAccountInfo = defaultAccounts.find(account => connectingAccounts.includes(account.address.toLowerCase()))
      const foundDefaultAccount = accounts.find((account) => account.address.toLowerCase() === foundDefaultAccountInfo?.address?.toLowerCase() ?? '')
      if (foundDefaultAccount) {
        setSelectedAccounts([foundDefaultAccount])
      }
    }
  }, [selectedPanel, defaultAccounts, connectingAccounts, accounts])

  React.useEffect(() => {
    if (hasIncorrectPassword) {
      // Clear the incorrect password
      setInputValue('')
    }
  }, [hasIncorrectPassword])

  const onSetBuyAmount = (value: string) => {
    setBuyAmount(value)
  }

  const onSubmitBuy = () => {
    GetBuyOrFaucetUrl(selectedNetwork.chainId, selectedWyreAsset, selectedAccount, buyAmount)
      .then(url => {
        chrome.tabs.create({ url }, () => {
          if (chrome.runtime.lastError) {
            console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
          }
        })
      })
      .catch(e => console.error(e))
  }

  const onChangeSendView = (view: BuySendSwapViewTypes) => {
    if (view === 'assets') {
      setShowSelectAsset(true)
    }
  }

  const onChangeSwapView = (view: BuySendSwapViewTypes, option?: ToOrFromType) => {
    if (view === 'assets') {
      setShowSelectAsset(true)
    }

    if (option) {
      setSwapToOrFrom(option)
    }
  }

  const onHideSelectAsset = () => {
    setShowSelectAsset(false)
  }

  const onSelectAsset = (asset: BraveWallet.BlockchainToken) => () => {
    if (selectedPanel === 'buy') {
      setSelectedWyreAsset(asset)
    } else if (selectedPanel === 'swap') {
      onSelectTransactAsset(asset, swapToOrFrom)
    } else {
      onSelectSendAsset(asset)
    }

    setShowSelectAsset(false)
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
    props.walletPanelActions.connectToSite({ selectedAccounts })
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
      props.walletPanelActions.cancelConnectToSite({ selectedAccounts })
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
    props.walletPanelActions.expandRestoreWallet()
  }
  const onSetup = () => {
    props.walletPanelActions.setupWallet()
  }
  const addToFavorites = (app: BraveWallet.AppItem) => {
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

  const removeFromFavorites = (app: BraveWallet.AppItem) => {
    props.walletActions.removeFavoriteApp(app)
  }

  const filterList = (event: any) => {
    filterAppList(event, AppsList(), setFilteredAppsList)
  }

  const onSelectAccount = (account: WalletAccountType) => () => {
    props.walletActions.selectAccount(account)
    props.walletPanelActions.navigateTo('main')
  }

  const onReturnToMain = () => {
    props.walletPanelActions.navigateTo('main')
  }

  const onCancelSigning = () => {
    if (isHardwareAccount(accounts, signMessageData[0].address)) {
      props.walletPanelActions.signMessageHardwareProcessed({
        success: false,
        id: signMessageData[0].id,
        signature: '',
        error: ''
      })
    } else {
      props.walletPanelActions.signMessageProcessed({
        approved: false,
        id: signMessageData[0].id
      })
    }
  }

  const onSignData = () => {
    if (isHardwareAccount(accounts, signMessageData[0].address)) {
      props.walletPanelActions.signMessageHardware(signMessageData[0])
    } else {
      props.walletPanelActions.signMessageProcessed({
        approved: true,
        id: signMessageData[0].id
      })
    }
  }

  const onApproveAddNetwork = () => {
    props.walletPanelActions.addEthereumChainRequestCompleted({ chainId: addChainRequest.networkInfo.chainId, approved: true })
  }

  const onCancelAddNetwork = () => {
    props.walletPanelActions.addEthereumChainRequestCompleted({ chainId: addChainRequest.networkInfo.chainId, approved: false })
  }

  const onApproveChangeNetwork = () => {
    props.walletPanelActions.switchEthereumChainProcessed({ approved: true, origin: switchChainRequest.originInfo.origin })
  }

  const onCancelChangeNetwork = () => {
    props.walletPanelActions.switchEthereumChainProcessed({ approved: false, origin: switchChainRequest.originInfo.origin })
  }

  const onNetworkLearnMore = () => {
    chrome.tabs.create({
      url: 'https://support.brave.com/'
    }).catch((e) => { console.error(e) })
  }

  const onRejectTransaction = () => {
    if (selectedPendingTransaction) {
      props.walletActions.rejectTransaction(selectedPendingTransaction)
    }
  }

  const retryHardwareOperation = () => {
    // signMessageData by default initialized as [{ id: -1, address: '', message: '' }]
    if (signMessageData && signMessageData.length && signMessageData[0].id !== -1) {
      onSignData()
    }
    if (selectedPendingTransaction) {
      onConfirmTransaction()
    }
  }
  const onConfirmTransaction = () => {
    if (!selectedPendingTransaction) {
      return
    }
    if (isHardwareAccount(accounts, selectedPendingTransaction.fromAddress)) {
      props.walletPanelActions.approveHardwareTransaction(selectedPendingTransaction)
    } else {
      props.walletActions.approveTransaction(selectedPendingTransaction)
      onSelectTransaction(selectedPendingTransaction)
    }
  }

  const onOpenSettings = () => {
    props.walletPanelActions.openWalletSettings()
  }

  const onCancelConnectHardwareWallet = () => {
    if (!selectedPendingTransaction) {
      return
    }
    props.walletPanelActions.cancelConnectHardwareWallet(selectedPendingTransaction)
  }

  const removeSitePermission = (origin: Origin, address: string, connectedAccounts: WalletAccountType[]) => {
    props.walletActions.removeSitePermission({ origin: origin, account: address })
    if (connectedAccounts.length !== 0) {
      props.walletActions.selectAccount(connectedAccounts[0])
    }
  }

  const addSitePermission = (origin: Origin, account: WalletAccountType) => {
    props.walletActions.addSitePermission({ origin: origin, account: account.address })
    props.walletActions.selectAccount(account)
  }

  const onSwitchAccount = (account: WalletAccountType) => {
    props.walletActions.selectAccount(account)
  }

  const onAddAccount = () => {
    props.walletPanelActions.expandWalletAccounts()
  }

  const onAddAsset = () => {
    props.walletPanelActions.expandWalletAddAsset()
  }

  const onAddSuggestedToken = () => {
    if (!suggestedTokenRequest) {
      return
    }
    props.walletPanelActions.addSuggestTokenProcessed({ approved: true, contractAddress: suggestedTokenRequest.token.contractAddress })
  }

  const onCancelAddSuggestedToken = () => {
    if (!suggestedTokenRequest) {
      return
    }
    props.walletPanelActions.addSuggestTokenProcessed({ approved: false, contractAddress: suggestedTokenRequest.token.contractAddress })
  }

  const onAddNetwork = () => {
    props.walletActions.expandWalletNetworks()
  }

  const onClickInstructions = () => {
    const url = 'https://support.brave.com/hc/en-us/articles/4409309138701'

    chrome.tabs.create({ url }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  const onSelectTransaction = (transaction: BraveWallet.TransactionInfo) => {
    props.walletPanelActions.setSelectedTransaction(transaction)
    props.walletPanelActions.navigateTo('transactionDetails')
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

  const onGoBackToTransactions = () => {
    props.walletPanelActions.navigateTo('transactions')
  }

  const onProvideEncryptionKey = () => {
    props.walletPanelActions.getEncryptionPublicKeyProcessed({ approved: true, origin: getEncryptionPublicKeyRequest.originInfo.origin })
  }

  const onCancelProvideEncryptionKey = () => {
    props.walletPanelActions.getEncryptionPublicKeyProcessed({ approved: false, origin: getEncryptionPublicKeyRequest.originInfo.origin })
  }

  const onAllowReadingEncryptedMessage = () => {
    props.walletPanelActions.decryptProcessed({ approved: true, origin: decryptRequest.originInfo.origin })
  }

  const onCancelAllowReadingEncryptedMessage = () => {
    props.walletPanelActions.decryptProcessed({ approved: false, origin: decryptRequest.originInfo.origin })
  }

  const isConnectedToSite = React.useMemo((): boolean => {
    if (activeOrigin.originSpec === WalletOrigin) {
      return true
    } else {
      return connectedAccounts.some(account => account.address === selectedAccount.address)
    }
  }, [connectedAccounts, selectedAccount, activeOrigin])

  if (!hasInitialized || !accounts) {
    return null
  }

  if (!isWalletCreated) {
    return (
      <WelcomePanelWrapper>
        <LongWrapper>
          <WelcomePanel onSetup={onSetup} />
        </LongWrapper>
      </WelcomePanelWrapper>
    )
  }

  if (isWalletLocked) {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <LockPanel
            value={inputValue}
            hasPasswordError={hasIncorrectPassword}
            onSubmit={unlockWallet}
            disabled={inputValue === ''}
            onPasswordChanged={handlePasswordChanged}
            onClickRestore={onRestore}
          />
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if ((selectedPendingTransaction || signMessageData.length) &&
    selectedPanel === 'connectHardwareWallet') {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <ConnectHardwareWalletPanel
            onCancel={onCancelConnectHardwareWallet}
            walletName={selectedAccount.name}
            hardwareWalletCode={props.panel.hardwareWalletCode}
            retryCallable={retryHardwareOperation}
            onClickInstructions={onClickInstructions}
          />
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPendingTransaction) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <ConfirmTransactionPanel
            onConfirm={onConfirmTransaction}
            onReject={onRejectTransaction}
          />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'addSuggestedToken') {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <AddSuggestedTokenPanel
            onCancel={onCancelAddSuggestedToken}
            onAddToken={onAddSuggestedToken}
            originInfo={suggestedTokenRequest?.origin ?? activeOrigin}
            token={suggestedTokenRequest?.token}
            selectedNetwork={selectedNetwork}
          />
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'addEthereumChain') {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <AllowAddChangeNetworkPanel
            originInfo={addChainRequest.originInfo}
            onApproveAddNetwork={onApproveAddNetwork}
            onApproveChangeNetwork={onApproveChangeNetwork}
            onCancel={onCancelAddNetwork}
            onLearnMore={onNetworkLearnMore}
            networkPayload={addChainRequest.networkInfo}
            panelType='add'
          />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'switchEthereumChain') {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <AllowAddChangeNetworkPanel
            originInfo={switchChainRequest.originInfo}
            onApproveAddNetwork={onApproveAddNetwork}
            onApproveChangeNetwork={onApproveChangeNetwork}
            onCancel={onCancelChangeNetwork}
            onLearnMore={onNetworkLearnMore}
            networkPayload={getNetworkInfo(switchChainRequest.chainId, networkList)}
            panelType='change'
          />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'signData') {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <SignPanel
            signMessageData={signMessageData}
            accounts={accounts}
            onCancel={onCancelSigning}
            onSign={onSignData}
            selectedNetwork={getNetworkInfo(selectedNetwork.chainId, networkList)}
            // Pass a boolean here if the signing method is risky
            showWarning={false}
          />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (
    selectedPanel === 'provideEncryptionKey' ||
    selectedPanel === 'allowReadingEncryptedMessage'
  ) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <EncryptionKeyPanel
            panelType={
              selectedPanel === 'provideEncryptionKey'
                ? 'request'
                : 'read'
            }
            encryptionKeyPayload={getEncryptionPublicKeyRequest}
            decryptPayload={decryptRequest}
            accounts={accounts}
            selectedNetwork={selectedNetwork}
            eTldPlusOne={activeOrigin.eTldPlusOne}
            onCancel={
              selectedPanel === 'provideEncryptionKey'
                ? onCancelProvideEncryptionKey
                : onCancelAllowReadingEncryptedMessage
            }
            onProvideOrAllow={
              selectedPanel === 'provideEncryptionKey'
                ? onProvideEncryptionKey
                : onAllowReadingEncryptedMessage
            }
          />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (showSelectAsset) {
    let assets: BraveWallet.BlockchainToken[]
    if (selectedPanel === 'buy') {
      assets = buyAssetOptions
    } else if (selectedPanel === 'send') {
      assets = sendAssetOptions
    } else { // swap
      assets = filteredAssetList
    }
    return (
      <PanelWrapper isLonger={false}>
        <SelectContainer>
          <SelectAsset
            assets={assets}
            onSelectAsset={onSelectAsset}
            onBack={onHideSelectAsset}
          />
        </SelectContainer>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'networks') {
    return (
      <PanelWrapper isLonger={false}>
        <SelectContainer>
          <SelectNetworkWithHeader
            onBack={onReturnToMain}
            onAddNetwork={onAddNetwork}
            hasAddButton={true}
          />
        </SelectContainer>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'accounts') {
    return (
      <PanelWrapper isLonger={false}>
        <SelectContainer>
          <SelectAccount
            accounts={accounts}
            onBack={onReturnToMain}
            onSelectAccount={onSelectAccount}
            onAddAccount={onAddAccount}
            selectedAccount={selectedAccount}
            hasAddButton={true}
          />
        </SelectContainer>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'apps') {
    return (
      <PanelWrapper isLonger={false}>
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
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'connectWithSite') {
    const accountsToConnect = props.wallet.accounts.filter(
      (account) => props.panel.connectingAccounts.includes(account.address.toLowerCase())
    )
    return (
      <PanelWrapper isLonger={true}>
        <ConnectWithSiteWrapper>
          <ConnectWithSite
            originInfo={connectToSiteOrigin}
            isReady={readyToConnect}
            accounts={accountsToConnect}
            primaryAction={primaryAction}
            secondaryAction={secondaryAction}
            selectAccount={selectAccount}
            removeAccount={removeAccount}
            selectedAccounts={selectedAccounts}
          />
        </ConnectWithSiteWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'send') {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <Panel
            navAction={navigateTo}
            title={panelTitle}
            useSearch={false}
          >
            <SendWrapper>
              <Send
                onChangeSendView={onChangeSendView}
              />
            </SendWrapper>
          </Panel>
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'buy') {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <Panel
            navAction={navigateTo}
            title={panelTitle}
            useSearch={false}
          >
            <SendWrapper>
              <Buy
                defaultCurrencies={defaultCurrencies}
                onChangeBuyView={onChangeSendView}
                onInputChange={onSetBuyAmount}
                onSubmit={onSubmitBuy}
                selectedAsset={selectedWyreAsset}
                buyAmount={buyAmount}
                selectedNetwork={getNetworkInfo(selectedNetwork.chainId, networkList)}
                networkList={networkList}
              />
            </SendWrapper>
          </Panel>
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'swap') {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <Panel
            navAction={navigateTo}
            title={panelTitle}
            useSearch={false}
          >
            <SendWrapper>
              <Swap
                {...swap}
                onChangeSwapView={onChangeSwapView}
              />
            </SendWrapper>
          </Panel>
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'transactionDetails' && selectedTransaction) {
    return (
      <PanelWrapper isLonger={false}>
        <SelectContainer>
          <TransactionDetailPanel
            onCancelTransaction={onCancelTransaction}
            onRetryTransaction={onRetryTransaction}
            onSpeedupTransaction={onSpeedupTransaction}
            onBack={onGoBackToTransactions}
            accounts={accounts}
            defaultCurrencies={defaultCurrencies}
            selectedNetwork={selectedNetwork}
            transaction={selectedTransaction}
            transactionSpotPrices={transactionSpotPrices}
            visibleTokens={userVisibleTokensInfo}
          />
        </SelectContainer>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'transactions') {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <Panel
            navAction={navigateTo}
            title={panelTitle}
            useSearch={false}
          >
            <ScrollContainer>
              <TransactionsPanel
                accounts={accounts}
                defaultCurrencies={defaultCurrencies}
                onSelectTransaction={onSelectTransaction}
                selectedNetwork={selectedNetwork}
                selectedAccount={selectedAccount}
                visibleTokens={userVisibleTokensInfo}
                transactionSpotPrices={transactionSpotPrices}
                transactions={transactions}
              />
            </ScrollContainer>
          </Panel>
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'assets') {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <Panel
            navAction={navigateTo}
            title={panelTitle}
            useSearch={false}
          >
            <ScrollContainer>
              <AssetsPanel
                defaultCurrencies={defaultCurrencies}
                selectedAccount={selectedAccount}
                userAssetList={panelUserAssetList}
                spotPrices={transactionSpotPrices}
                networkList={networkList}
                onAddAsset={onAddAsset}
              />
            </ScrollContainer>
          </Panel>
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'sitePermissions') {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <Panel
            navAction={navigateTo}
            title={panelTitle}
            useSearch={false}
          >
            <SitePermissions
              accounts={accounts}
              connectedAccounts={connectedAccounts}
              onConnect={addSitePermission}
              onDisconnect={removeSitePermission}
              onSwitchAccount={onSwitchAccount}
              selectedAccount={selectedAccount}
              originInfo={activeOrigin}
              onAddAccount={onAddAccount}
            />
          </Panel>
        </LongWrapper>
      </PanelWrapper>
    )
  }

  return (
    <PanelWrapper isLonger={false}>
      <ConnectedPanel
        defaultCurrencies={defaultCurrencies}
        spotPrices={transactionSpotPrices}
        selectedAccount={selectedAccount}
        selectedNetwork={getNetworkInfo(selectedNetwork.chainId, networkList)}
        isConnected={isConnectedToSite}
        navAction={navigateTo}
        onLockWallet={onLockWallet}
        onOpenSettings={onOpenSettings}
        originInfo={activeOrigin}
        isSwapSupported={isSwapSupported}
      />
    </PanelWrapper>
  )
}

export default connect(mapStateToProps, mapDispatchToProps)(Container)
