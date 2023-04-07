// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

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
import { CreateAccountTab } from '../components/buy-send-swap/create-account'
import { SelectNetworkWithHeader } from '../components/buy-send-swap/select-network-with-header'
import { SelectAccountWithHeader } from '../components/buy-send-swap/select-account-with-header'
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
  PanelWrapper,
  WelcomePanelWrapper
} from './style'

import * as WalletPanelActions from './actions/wallet_panel_actions'
import * as WalletActions from '../common/actions/wallet_actions'
import {
  AppsListType,
  BraveWallet,
  PanelTypes,
  WalletAccountType,
  SerializableTransactionInfo
} from '../constants/types'

import { AppsList } from '../options/apps-list-options'
import LockPanel from '../components/extension/lock-panel'
import { getCoinFromTxDataUnion } from '../utils/network-utils'
import { isHardwareAccount } from '../utils/address-utils'
import { useAssets, useHasAccount, usePrevNetwork, useBalanceUpdater } from '../common/hooks'
import { findTransactionAccount, isSolanaTransaction } from '../utils/tx-utils'
import { ConfirmSolanaTransactionPanel } from '../components/extension/confirm-transaction-panel/confirm-solana-transaction-panel'
import { SignTransactionPanel } from '../components/extension/sign-panel/sign-transaction-panel'
import { useDispatch } from 'react-redux'
import { SelectCurrency } from '../components/buy-send-swap/select-currency/select-currency'
import { ConfirmSwapTransaction } from '../components/extension/confirm-transaction-panel/swap'
import { TransactionStatus } from '../components/extension/post-confirmation'
import { useSafePanelSelector, useSafeWalletSelector, useUnsafePanelSelector, useUnsafeWalletSelector } from '../common/hooks/use-safe-selector'
import { WalletSelectors } from '../common/selectors'
import { PanelSelectors } from './selectors'
import {
  useGetNetworkQuery,
  useGetSelectedChainQuery
} from '../common/slices/api.slice'

// Allow BigInts to be stringified
(BigInt.prototype as any).toJSON = function () {
  return this.toString()
}

function Container () {
  // redux
  const dispatch = useDispatch()

  // wallet selectors (safe)
  const hasInitialized = useSafeWalletSelector(WalletSelectors.hasInitialized)
  const isWalletCreated = useSafeWalletSelector(WalletSelectors.isWalletCreated)
  const isWalletLocked = useSafeWalletSelector(WalletSelectors.isWalletLocked)

  // wallet selectors (unsafe)
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const activeOrigin = useUnsafeWalletSelector(WalletSelectors.activeOrigin)
  const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)
  const favoriteApps = useUnsafeWalletSelector(WalletSelectors.favoriteApps)
  const selectedAccount = useUnsafeWalletSelector(WalletSelectors.selectedAccount)
  const selectedPendingTransaction = useUnsafeWalletSelector(WalletSelectors.selectedPendingTransaction)
  const transactionSpotPrices = useUnsafeWalletSelector(WalletSelectors.transactionSpotPrices)
  const userVisibleTokensInfo = useUnsafeWalletSelector(WalletSelectors.userVisibleTokensInfo)

  // panel selectors (safe)
  const panelTitle = useSafePanelSelector(PanelSelectors.panelTitle)
  const selectedPanel = useSafePanelSelector(PanelSelectors.selectedPanel)
  const hardwareWalletCode = useSafePanelSelector(PanelSelectors.hardwareWalletCode)

  // panel selectors (unsafe)
  const connectToSiteOrigin = useUnsafePanelSelector(PanelSelectors.connectToSiteOrigin)
  const addChainRequest = useUnsafePanelSelector(PanelSelectors.addChainRequest)
  const signMessageData = useUnsafePanelSelector(PanelSelectors.signMessageData)
  const switchChainRequest = useUnsafePanelSelector(PanelSelectors.switchChainRequest)
  const suggestedTokenRequest = useUnsafePanelSelector(PanelSelectors.suggestedTokenRequest)
  const getEncryptionPublicKeyRequest = useUnsafePanelSelector(PanelSelectors.getEncryptionPublicKeyRequest)
  const decryptRequest = useUnsafePanelSelector(PanelSelectors.decryptRequest)
  const connectingAccounts = useUnsafePanelSelector(PanelSelectors.connectingAccounts)
  const selectedTransaction = useUnsafePanelSelector(PanelSelectors.selectedTransaction)

  // queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()
  const { data: switchChainRequestNetwork } = useGetNetworkQuery(
    {
      chainId: switchChainRequest.chainId,
      // Passed ETH here since AllowAddChangeNetworkPanel
      // is only used for EVM networks
      // and switchChainRequest doesn't return coinType.
      coin: BraveWallet.CoinType.ETH
    },
    { skip: !switchChainRequest.chainId }
  )

  // TODO(petemill): If initial data or UI takes a noticeable amount of time to arrive
  // consider rendering a "loading" indicator when `hasInitialized === false`, and
  // also using `React.lazy` to put all the main UI in a separate JS bundle and display
  // that loading indicator ASAP.
  const [filteredAppsList, setFilteredAppsList] = React.useState<AppsListType[]>(AppsList)

  const {
    panelUserAssetList
  } = useAssets()

  // hooks
  useBalanceUpdater()

  const { needsAccount } = useHasAccount()

  const { prevNetwork } = usePrevNetwork()

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
    dispatch(WalletPanelActions.setSelectedTransaction(undefined))
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

  const viewTransactionDetail = React.useCallback((transaction: SerializableTransactionInfo) => {
    dispatch(WalletPanelActions.setSelectedTransaction(transaction))
    dispatch(WalletPanelActions.navigateTo('transactionDetails'))
  }, [])

  const viewTransactionStatus = React.useCallback((transaction: SerializableTransactionInfo) => {
    dispatch(WalletPanelActions.setSelectedTransaction(transaction))
    dispatch(WalletPanelActions.navigateTo('transactionStatus'))
  }, [])

  const onConfirmTransaction = () => {
    if (!selectedPendingTransaction) {
      return
    }
    if (isHardwareAccount(accounts, selectedPendingTransaction.fromAddress)) {
      dispatch(WalletPanelActions.approveHardwareTransaction(selectedPendingTransaction))
    } else {
      dispatch(WalletActions.approveTransaction(selectedPendingTransaction))
      viewTransactionStatus(selectedPendingTransaction)
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

  const onRetryTransaction = (transaction: SerializableTransactionInfo) => {
    dispatch(WalletActions.retryTransaction({
      coinType: getCoinFromTxDataUnion(transaction.txDataUnion),
      fromAddress: findTransactionAccount(accounts, transaction)?.address || '',
      transactionId: transaction.id
    }))
  }

  const onSpeedupTransaction = (transaction: SerializableTransactionInfo) => {
    dispatch(WalletActions.speedupTransaction({
      coinType: getCoinFromTxDataUnion(transaction.txDataUnion),
      fromAddress: findTransactionAccount(accounts, transaction)?.address || '',
      transactionId: transaction.id
    }))
  }

  const onCancelTransaction = (transaction: SerializableTransactionInfo) => {
    dispatch(WalletActions.cancelTransaction({
      coinType: getCoinFromTxDataUnion(transaction.txDataUnion),
      fromAddress: findTransactionAccount(accounts, transaction)?.address || '',
      transactionId: transaction.id
    }))
  }

  const onGoBackToTransactions = () => {
    dispatch(WalletPanelActions.navigateBack())
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
    navigateTo('buy')
  }, [])

  const onSelectCurrency = React.useCallback(() => {
    dispatch(WalletPanelActions.navigateTo('buy'))
  }, [])

  React.useEffect(() => {
    if (needsAccount) {
      dispatch(WalletPanelActions.navigateTo('createAccount'))
    }
  }, [needsAccount])

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

  if (selectedPanel === 'transactionStatus' && selectedTransaction) {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <TransactionStatus
            transaction={selectedTransaction}
          />
        </StyledExtensionWrapper>
      </PanelWrapper>
    )
  }

  if (selectedAccount && (selectedPendingTransaction || signMessageData.length) &&
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

  if (selectedPendingTransaction?.txType === BraveWallet.TransactionType.ETHSwap) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <ConfirmSwapTransaction
            onConfirm={onConfirmTransaction}
            onReject={onRejectTransaction}
          />
        </LongWrapper>
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

  if (selectedPanel === 'switchEthereumChain' && switchChainRequestNetwork) {
    return (
      <PanelWrapper isLonger={true}>
        <LongWrapper>
          <AllowAddChangeNetworkPanel
            originInfo={switchChainRequest.originInfo}
            onApproveAddNetwork={onApproveAddNetwork}
            onApproveChangeNetwork={onApproveChangeNetwork}
            onCancel={onCancelChangeNetwork}
            networkPayload={switchChainRequestNetwork}
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
            selectedNetwork={selectedNetwork}
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
          <SelectAccountWithHeader
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
      <PanelWrapper width={390} height={600}>
        <ConnectWithSiteWrapper>
          <ConnectWithSite
            originInfo={connectToSiteOrigin}
            accountsToConnect={accountsToConnect}
          />
        </ConnectWithSiteWrapper>
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

  // Transactions
  if (selectedPanel === 'activity') {
    return (
      <PanelWrapper isLonger={false}>
        <StyledExtensionWrapper>
          <Panel
            navAction={navigateTo}
            title={panelTitle}
            useSearch={false}
          >
            <TransactionsPanel
              onSelectTransaction={viewTransactionDetail}
              selectedNetwork={selectedNetwork}
              selectedAccountAddress={selectedAccount?.address}
            />
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
                selectedAccount={selectedAccount}
                userAssetList={panelUserAssetList}
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
      />
    </PanelWrapper>
  )
}

export default Container
