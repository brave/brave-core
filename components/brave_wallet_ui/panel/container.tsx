// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
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
  Swap,
  CreateAccountTab
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

import * as WalletPanelActions from './actions/wallet_panel_actions'
import * as WalletActions from '../common/actions/wallet_actions'
import {
  AppsListType,
  BraveWallet,
  WalletState,
  PanelState,
  PanelTypes,
  WalletAccountType,
  BuySendSwapViewTypes,
  ToOrFromType
} from '../constants/types'

import { AppsList } from '../options/apps-list-options'
import LockPanel from '../components/extension/lock-panel'
import { getNetworkInfo } from '../utils/network-utils'
import { isHardwareAccount } from '../utils/address-utils'
import { useAssets, useSwap, useSend, useHasAccount, usePrevNetwork } from '../common/hooks'
import { getUniqueAssets } from '../utils/asset-utils'
import { isSolanaTransaction } from '../utils/tx-utils'
import { ConfirmSolanaTransactionPanel } from '../components/extension/confirm-transaction-panel/confirm-solana-transaction-panel'
import { SignTransactionPanel } from '../components/extension/sign-panel/sign-transaction-panel'
import { useDispatch, useSelector } from 'react-redux'
import { SelectCurrency } from '../components/buy-send-swap/select-currency/select-currency'

// Allow BigInts to be stringified
(BigInt.prototype as any).toJSON = function () {
  return this.toString()
}

function Container () {
  // redux
  const dispatch = useDispatch()
  const {
    accounts,
    selectedAccount,
    selectedNetwork,
    selectedPendingTransaction,
    isWalletLocked,
    favoriteApps,
    hasInitialized,
    isWalletCreated,
    networkList,
    transactionSpotPrices,
    activeOrigin,
    defaultCurrencies,
    transactions,
    userVisibleTokensInfo,
    defaultAccounts,
    defaultNetworks
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)
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
    selectedTransaction,
    hardwareWalletCode
  } = useSelector(({ panel }: { panel: PanelState }) => panel)

  // TODO(petemill): If initial data or UI takes a noticeable amount of time to arrive
  // consider rendering a "loading" indicator when `hasInitialized === false`, and
  // also using `React.lazy` to put all the main UI in a separate JS bundle and display
  // that loading indicator ASAP.
  const [selectedAccounts, setSelectedAccounts] = React.useState<WalletAccountType[]>([])
  const [filteredAppsList, setFilteredAppsList] = React.useState<AppsListType[]>(AppsList)
  const [showSelectAsset, setShowSelectAsset] = React.useState<boolean>(false)

  const {
    sendAssetOptions,
    buyAssetOptions,
    panelUserAssetList
  } = useAssets()

  const [selectedBuyAsset, setSelectedBuyAsset] = React.useState<BraveWallet.BlockchainToken>(buyAssetOptions[0])

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

  const { needsAccount } = useHasAccount()

  const { prevNetwork } = usePrevNetwork()

  React.useEffect(() => {
    // Checking selectedAccounts length here to ensure that
    // we only update this once on mount.
    if (selectedPanel === 'connectWithSite' && selectedAccounts.length === 0) {
      const foundDefaultAccountInfo = defaultAccounts.find(account => connectingAccounts.includes(account.address.toLowerCase()))
      const foundDefaultAccount = accounts.find((account) => account.address.toLowerCase() === foundDefaultAccountInfo?.address?.toLowerCase() ?? '')

      if (foundDefaultAccount) {
        setSelectedAccounts([foundDefaultAccount])
      }
    }
  }, [selectedPanel, defaultAccounts, connectingAccounts, accounts])

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
      setSelectedBuyAsset(asset)
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
  const onSubmit = () => {
    dispatch(WalletPanelActions.connectToSite({ selectedAccounts }))
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
      dispatch(WalletPanelActions.cancelConnectToSite({ selectedAccounts }))
      setSelectedAccounts([])
      setReadyToConnect(false)
    }
  }

  const unlockWallet = (password: string) => {
    dispatch(WalletActions.unlockWallet({ password }))
  }

  const onRestore = () => {
    dispatch(WalletPanelActions.expandRestoreWallet())
  }

  const onSetup = () => {
    dispatch(WalletPanelActions.setupWallet())
  }

  const addToFavorites = (app: BraveWallet.AppItem) => {
    dispatch(WalletActions.addFavoriteApp(app))
  }

  const navigateTo = (path: PanelTypes) => {
    if (path === 'expanded') {
      dispatch(WalletPanelActions.expandWallet())
    } else {
      dispatch(WalletPanelActions.navigateTo(path))
    }
  }

  const browseMore = () => {
    dispatch(WalletPanelActions.openWalletApps())
  }

  const removeFromFavorites = (app: BraveWallet.AppItem) => {
    dispatch(WalletActions.removeFavoriteApp(app))
  }

  const filterList = (event: any) => {
    filterAppList(event, AppsList(), setFilteredAppsList)
  }

  const onSelectAccount = (account: WalletAccountType) => () => {
    dispatch(WalletActions.selectAccount(account))
    dispatch(WalletPanelActions.navigateTo('main'))
  }

  const onReturnToMain = () => {
    dispatch(WalletPanelActions.navigateTo('main'))
  }

  const onCancelSigning = () => {
    dispatch(WalletPanelActions.signMessageProcessed({
      approved: false,
      id: signMessageData[0].id
    }))
  }

  const onSignData = () => {
    if (isHardwareAccount(accounts, signMessageData[0].address)) {
      dispatch(WalletPanelActions.signMessageHardware(signMessageData[0]))
    } else {
      dispatch(WalletPanelActions.signMessageProcessed({
        approved: true,
        id: signMessageData[0].id
      }))
    }
  }

  const onApproveAddNetwork = () => {
    dispatch(WalletPanelActions.addEthereumChainRequestCompleted({ chainId: addChainRequest.networkInfo.chainId, approved: true }))
  }

  const onCancelAddNetwork = () => {
    dispatch(WalletPanelActions.addEthereumChainRequestCompleted({ chainId: addChainRequest.networkInfo.chainId, approved: false }))
  }

  const onApproveChangeNetwork = () => {
    dispatch(WalletPanelActions.switchEthereumChainProcessed({ approved: true, origin: switchChainRequest.originInfo.origin }))
  }

  const onCancelChangeNetwork = () => {
    dispatch(WalletPanelActions.switchEthereumChainProcessed({ approved: false, origin: switchChainRequest.originInfo.origin }))
  }

  const onRejectTransaction = () => {
    if (selectedPendingTransaction) {
      dispatch(WalletActions.rejectTransaction(selectedPendingTransaction))
    }
  }

  const onSelectTransaction = (transaction: BraveWallet.TransactionInfo) => {
    dispatch(WalletPanelActions.setSelectedTransaction(transaction))
    dispatch(WalletPanelActions.navigateTo('transactionDetails'))
  }

  const onConfirmTransaction = () => {
    if (!selectedPendingTransaction) {
      return
    }
    if (isHardwareAccount(accounts, selectedPendingTransaction.fromAddress)) {
      dispatch(WalletPanelActions.approveHardwareTransaction(selectedPendingTransaction))
    } else {
      dispatch(WalletActions.approveTransaction(selectedPendingTransaction))
      onSelectTransaction(selectedPendingTransaction)
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

  const onCancelConnectHardwareWallet = (accountAddress: string, coinType: BraveWallet.CoinType) => {
    dispatch(WalletPanelActions.cancelConnectHardwareWallet({ accountAddress, coinType }))
  }

  const onAddAccount = () => {
    dispatch(WalletPanelActions.expandWalletAccounts())
  }

  const onAddAsset = () => {
    dispatch(WalletPanelActions.expandWalletAddAsset())
  }

  const onAddSuggestedToken = () => {
    if (!suggestedTokenRequest) {
      return
    }
    dispatch(WalletPanelActions.addSuggestTokenProcessed({ approved: true, contractAddress: suggestedTokenRequest.token.contractAddress }))
  }

  const onCancelAddSuggestedToken = () => {
    if (!suggestedTokenRequest) {
      return
    }
    dispatch(WalletPanelActions.addSuggestTokenProcessed({ approved: false, contractAddress: suggestedTokenRequest.token.contractAddress }))
  }

  const onAddNetwork = () => {
    dispatch(WalletActions.expandWalletNetworks())
  }

  const onClickInstructions = () => {
    const url = 'https://support.brave.com/hc/en-us/articles/4409309138701'

    chrome.tabs.create({ url }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  const onRetryTransaction = (transaction: BraveWallet.TransactionInfo) => {
    dispatch(WalletActions.retryTransaction(transaction))
  }

  const onSpeedupTransaction = (transaction: BraveWallet.TransactionInfo) => {
    dispatch(WalletActions.speedupTransaction(transaction))
  }

  const onCancelTransaction = (transaction: BraveWallet.TransactionInfo) => {
    dispatch(WalletActions.cancelTransaction(transaction))
  }

  const onGoBackToTransactions = () => {
    dispatch(WalletPanelActions.navigateTo('transactions'))
  }

  const onProvideEncryptionKey = () => {
    dispatch(WalletPanelActions.getEncryptionPublicKeyProcessed({ approved: true, origin: getEncryptionPublicKeyRequest.originInfo.origin }))
  }

  const onCancelProvideEncryptionKey = () => {
    dispatch(WalletPanelActions.getEncryptionPublicKeyProcessed({ approved: false, origin: getEncryptionPublicKeyRequest.originInfo.origin }))
  }

  const onAllowReadingEncryptedMessage = () => {
    dispatch(WalletPanelActions.decryptProcessed({ approved: true, origin: decryptRequest.originInfo.origin }))
  }

  const onCancelAllowReadingEncryptedMessage = () => {
    dispatch(WalletPanelActions.decryptProcessed({ approved: false, origin: decryptRequest.originInfo.origin }))
  }

  const onBack = React.useCallback(() => {
    dispatch(WalletPanelActions.navigateTo('buy'))
  }, [])

  const onShowCurrencySelection = React.useCallback(() => {
    dispatch(WalletPanelActions.navigateTo('currencies'))
  }, [])

  const onSelectCurrency = React.useCallback(() => {
    dispatch(WalletPanelActions.navigateTo('buy'))
  }, [])

  React.useEffect(() => {
    if (needsAccount) {
      dispatch(WalletPanelActions.navigateTo('createAccount'))
    }
  }, [needsAccount])

  React.useEffect(() => {
    if (buyAssetOptions.length > 0) {
      setSelectedBuyAsset(buyAssetOptions[0])
    }
  }, [buyAssetOptions])

  const filteredAssetOptions = React.useMemo(() => {
    return getUniqueAssets(buyAssetOptions)
  }, [buyAssetOptions])

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
            onSubmit={unlockWallet}
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
            accountAddress={selectedAccount.address}
            coinType={selectedAccount.coin}
            hardwareWalletCode={hardwareWalletCode}
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
          {isSolanaTransaction(selectedPendingTransaction)
            ? <ConfirmSolanaTransactionPanel
              onConfirm={onConfirmTransaction}
              onReject={onRejectTransaction}
            />
            : <ConfirmTransactionPanel
              onConfirm={onConfirmTransaction}
              onReject={onRejectTransaction}
            />
          }
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
            // Passed BraveWallet.CoinType.ETH here since AllowAddChangeNetworkPanel
            // is only used for EVM networks and switchChainRequest doesn't return cointType.
            networkPayload={getNetworkInfo(switchChainRequest.chainId, BraveWallet.CoinType.ETH, networkList)}
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
            selectedNetwork={getNetworkInfo(selectedNetwork.chainId, selectedNetwork.coin, networkList)}
            defaultNetworks={defaultNetworks}
            // Pass a boolean here if the signing method is risky
            showWarning={false}
          />
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'signTransaction' || selectedPanel === 'signAllTransactions') {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <SignTransactionPanel
            signMode={selectedPanel === 'signAllTransactions' ? 'signAllTxs' : 'signTx'}
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
      assets = filteredAssetOptions
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
    const accountsToConnect = accounts.filter(
      (account) => connectingAccounts.includes(account.address.toLowerCase())
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
                onChangeBuyView={onChangeSendView}
                selectedAsset={selectedBuyAsset}
                onShowCurrencySelection={onShowCurrencySelection}
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
            <SitePermissions />
          </Panel>
        </LongWrapper>
      </PanelWrapper>
    )
  }

  if (selectedPanel === 'createAccount') {
    return (
      <WelcomePanelWrapper>
        <LongWrapper>
          <CreateAccountTab
            prevNetwork={prevNetwork}
            isPanel={true}
          />
        </LongWrapper>
      </WelcomePanelWrapper>
    )
  }

  if (selectedPanel === 'currencies') {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <Panel
            navAction={navigateTo}
            title={panelTitle}
            useSearch={false}
          >
            <ScrollContainer>
              <SelectCurrency
                onBack={onBack}
                onSelectCurrency={onSelectCurrency}
              />
            </ScrollContainer>
          </Panel>
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  return (
    <PanelWrapper isLonger={false}>
      <ConnectedPanel
        navAction={navigateTo}
        isSwapSupported={isSwapSupported}
      />
    </PanelWrapper>
  )
}

export default Container
