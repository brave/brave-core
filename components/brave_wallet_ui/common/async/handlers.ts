// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletActions from '../actions/wallet_actions'
import {
  ChainChangedEventPayloadType,
  AddSitePermissionPayloadType,
  RemoveSitePermissionPayloadType,
  SetUserAssetVisiblePayloadType,
  UnlockWalletPayloadType,
  UpdateUnapprovedTransactionGasFieldsType,
  UpdateUnapprovedTransactionSpendAllowanceType,
  TransactionStatusChanged,
  UpdateUnapprovedTransactionNonceType,
  WalletDataInitializedPayload
} from '../constants/action_types'
import {
  BraveWallet,
  ApproveERC20Params,
  ER20TransferParams,
  ERC721TransferFromParams,
  SendEthTransactionParams,
  WalletAccountType,
  WalletState,
  WalletInfo,
  TransactionProviderError,
  SupportedCoinTypes,
  SendFilTransactionParams,
  SendSolTransactionParams,
  SPLTransferFromParams
} from '../../constants/types'

// Utils

import getAPIProxy from './bridge'
import {
  hasEIP1559Support,
  refreshNetworkInfo,
  refreshPortfolioPriceHistory,
  refreshTransactionHistory,
  refreshBalances,
  refreshVisibleTokenInfo,
  refreshPrices,
  sendEthTransaction,
  sendFilTransaction,
  sendSolTransaction,
  sendSPLTransaction,
  getRefreshedBalancesAndPrices,
  refreshBalancesPricesAndHistory,
  refreshWalletInfo,
  updateAccountInfo,
  getAllTokensList
} from './lib'
import { Store } from './types'
import InteractionNotifier from './interactionNotifier'
import BalanceUpdater from './balanceUpdater'
import { refreshSelectedAccount } from '../../utils/account-utils'
import { getCoinFromTxDataUnion } from '../../utils/network-utils'

const handler = new AsyncActionHandler()

const interactionNotifier = new InteractionNotifier()
const balanceUpdater = new BalanceUpdater()

function getWalletState (store: Store): WalletState {
  return store.getState().wallet
}

handler.on(WalletActions.initialize.getType(), async (store) => {
  alert(`h - ${WalletActions.initialize.getType()}`)
  console.log(`h - ${WalletActions.initialize.getType()}`)
  // Initialize active origin state.
  const { originInfo } = await getAPIProxy().braveWalletService.getActiveOrigin()
  store.dispatch(WalletActions.activeOriginChanged(originInfo))

  const walletInfo = await refreshWalletInfo(store.getState().wallet)
  store.dispatch(WalletActions.walletInfoUpdated(walletInfo))
  store.dispatch(WalletActions.ready())
})

handler.on([
  WalletActions.initialized.getType(),
  WalletActions.ready.getType()
], async (store: Store, payload?: WalletInfo) => {
  alert(`h - ${WalletActions.initialized.getType()}`)
  console.log(`h - ${WalletActions.initialized.getType()}`)
  const { keyringService, braveWalletService } = getAPIProxy()
  const state = getWalletState(store)

  if (!state.isWalletLocked) {
    keyringService.notifyUserInteraction()
  }

  balanceUpdater.beginUpdatingBalances(15000, async () => {
    const { accounts, selectedAccount } = await refreshBalances(store.getState().wallet)
    store.dispatch(WalletActions.accountsUpdated({ accounts, selectedAccount }))
  })

  interactionNotifier.beginWatchingForInteraction(50000, state.isWalletLocked, async () => {
    keyringService.notifyUserInteraction()
  })

  const { currency } = await braveWalletService.getDefaultBaseCurrency()
  const { cryptocurrency } = await braveWalletService.getDefaultBaseCryptocurrency()
  const defaultCurrencies = {
    fiat: currency,
    crypto: cryptocurrency
  }

  let walletDataInitializedPayload: Partial<WalletDataInitializedPayload> = { defaultCurrencies }

  let newState: WalletState = { ...state, defaultCurrencies }

  // Fetch Balances and Prices
  if (!state.isWalletLocked && state.isWalletCreated) {
    const { defaultNetworks, networkList, selectedNetwork } = await refreshNetworkInfo(state)
    newState = { ...state, defaultNetworks, networkList, selectedNetwork }
    walletDataInitializedPayload = { ...walletDataInitializedPayload, defaultNetworks, networkList, selectedNetwork }

    const userVisibleTokensInfo = await refreshVisibleTokenInfo(newState.networkList)
    newState.userVisibleTokensInfo = userVisibleTokensInfo
    walletDataInitializedPayload = { ...walletDataInitializedPayload, userVisibleTokensInfo }

    const { accounts, selectedAccount } = await refreshBalances(newState)
    newState = { ...newState, accounts, selectedAccount }
    walletDataInitializedPayload = { ...walletDataInitializedPayload, accounts, selectedAccount }

    const { success, values } = await refreshPrices(newState)
    const transactionSpotPrices = success ? values : newState.transactionSpotPrices
    newState.transactionSpotPrices = transactionSpotPrices
    walletDataInitializedPayload = { ...walletDataInitializedPayload, transactionSpotPrices }

    const {
      isFetchingPortfolioPriceHistory,
      portfolioPriceHistory
    } = await refreshPortfolioPriceHistory(newState)
    newState = { ...newState, isFetchingPortfolioPriceHistory, portfolioPriceHistory }
    walletDataInitializedPayload = { ...walletDataInitializedPayload, isFetchingPortfolioPriceHistory, portfolioPriceHistory }
  }

  // This can be 0 when the wallet is locked
  if (payload?.selectedAccount) {
    // await store.dispatch(refreshTransactionHistory(newState, payload.selectedAccount))
    const {
      pendingTransactions,
      selectedPendingTransaction,
      transactions
    } = await refreshTransactionHistory(newState, payload.selectedAccount)
    walletDataInitializedPayload = { ...walletDataInitializedPayload, pendingTransactions, selectedPendingTransaction, transactions }
  }

  store.dispatch(WalletActions.walletDataInitialized(walletDataInitializedPayload))
})

handler.on(WalletActions.refreshBalancesAndPrices.getType(), async (store: Store) => {
  alert(`h - ${WalletActions.refreshBalancesAndPrices.getType()}`)
  console.log(`h - ${WalletActions.refreshBalancesAndPrices.getType()}`)
  const refreshedData = await getRefreshedBalancesAndPrices(getWalletState(store))
  store.dispatch(WalletActions.balancesAndPricesRefreshed(refreshedData))
})

handler.on(WalletActions.defaultBaseCryptocurrencyChanged.getType(), async (store) => {
  alert(`h - ${WalletActions.defaultBaseCryptocurrencyChanged.getType()}`)
  console.log(`h - ${WalletActions.defaultBaseCryptocurrencyChanged.getType()}`)
  store.dispatch(WalletActions.walletInfoUpdated(await refreshWalletInfo(store.getState().wallet)))
})

handler.on(WalletActions.backedUp.getType(), async (store) => {
  alert(`h - ${WalletActions.backedUp.getType()}`)
  console.log(`h - ${WalletActions.backedUp.getType()}`)
  store.dispatch(WalletActions.walletInfoUpdated(await refreshWalletInfo(store.getState().wallet)))
})

handler.on(WalletActions.defaultWalletChanged.getType(), async (store) => {
  alert(`h - ${WalletActions.defaultWalletChanged.getType()}`)
  console.log(`h - ${WalletActions.defaultWalletChanged.getType()}`)
  store.dispatch(WalletActions.walletInfoUpdated(await refreshWalletInfo(store.getState().wallet)))
})

handler.on(WalletActions.keyringCreated.getType(), async (store) => {
  alert(`h - ${WalletActions.keyringCreated.getType()}`)
  console.log(`h - ${WalletActions.keyringCreated.getType()}`)
  store.dispatch(WalletActions.walletInfoUpdated(await refreshWalletInfo(store.getState().wallet)))
})

handler.on(WalletActions.keyringRestored.getType(), async (store) => {
  alert(`h - ${WalletActions.keyringRestored.getType()}`)
  console.log(`h - ${WalletActions.keyringRestored.getType()}`)
  store.dispatch(WalletActions.walletInfoUpdated(await refreshWalletInfo(store.getState().wallet)))
})

handler.on(WalletActions.unlocked.getType(), async (store) => {
  alert(`h - ${WalletActions.unlocked.getType()}`)
  console.log(`h - ${WalletActions.unlocked.getType()}`)
  store.dispatch(WalletActions.walletInfoUpdated(await refreshWalletInfo(store.getState().wallet)))
})

handler.on(WalletActions.defaultBaseCurrencyChanged.getType(), async (store) => {
  alert(`h - ${WalletActions.defaultBaseCurrencyChanged.getType()}`)
  console.log(`h - ${WalletActions.defaultBaseCurrencyChanged.getType()}`)
  store.dispatch(WalletActions.walletInfoUpdated(await refreshWalletInfo(store.getState().wallet)))
})

handler.on(WalletActions.keyringReset.getType(), async (store) => {
  alert(`h - ${WalletActions.keyringReset.getType()}`)
  console.log(`h - ${WalletActions.keyringReset.getType()}`)
  window.location.reload()
})

handler.on(WalletActions.locked.getType(), async (store) => {
  alert(`h - ${WalletActions.locked.getType()}`)
  console.log(`h - ${WalletActions.locked.getType()}`)
  interactionNotifier.stopWatchingForInteraction()
  balanceUpdater.stopUpdatingBalances()

  const payload = await refreshWalletInfo(store.getState().wallet)
  store.dispatch(WalletActions.walletInfoUpdated(payload))
})

handler.on(WalletActions.accountsChanged.getType(), async (store) => {
  alert(`h - ${WalletActions.accountsChanged.getType()}`)
  console.log(`h - ${WalletActions.accountsChanged.getType()}`)
  const walletState = getWalletState(store)
  const accounts = await updateAccountInfo(walletState)
  store.dispatch(WalletActions.accountsUpdated({
    accounts,
    selectedAccount: refreshSelectedAccount(accounts, walletState.selectedAccount)
  }))
})

// Will use this observer selectedAccountChanged action again once
// selectedCoin is implemented here https://github.com/brave/brave-browser/issues/21465

// handler.on(WalletActions.selectedAccountChanged.getType(), async (store) => {
  alert(`h - ${WalletActions.selectedAccountChanged.getType()}`)
  console.log(`h - ${WalletActions.selectedAccountChanged.getType()}`)
//   await refreshWalletInfo(store)
// })

handler.on(WalletActions.lockWallet.getType(), async (store) => {
  alert(`h - ${WalletActions.lockWallet.getType()}`)
  console.log(`h - ${WalletActions.lockWallet.getType()}`)
  getAPIProxy().keyringService.lock()
})

handler.on(WalletActions.unlockWallet.getType(), async (store: Store, payload: UnlockWalletPayloadType) => {
  alert(`h - ${WalletActions.unlockWallet.getType()}`)
  console.log(`h - ${WalletActions.unlockWallet.getType()}`)
  const result = await getAPIProxy().keyringService.unlock(payload.password)
  store.dispatch(WalletActions.hasIncorrectPassword(!result.success))
})

handler.on(WalletActions.addFavoriteApp.getType(), async (store: Store, appItem: BraveWallet.AppItem) => {
  alert(`h - ${WalletActions.addFavoriteApp.getType()}`)
  console.log(`h - ${WalletActions.addFavoriteApp.getType()}`)
  getAPIProxy().walletHandler.addFavoriteApp(appItem)
  store.dispatch(WalletActions.walletInfoUpdated(await refreshWalletInfo({
    ...store.getState().wallet,
    favoriteApps: [...store.getState().wallet.favoriteApps, appItem]
  })))
})

handler.on(WalletActions.removeFavoriteApp.getType(), async (store: Store, appItem: BraveWallet.AppItem) => {
  alert(`h - ${WalletActions.removeFavoriteApp.getType()}`)
  console.log(`h - ${WalletActions.removeFavoriteApp.getType()}`)
  getAPIProxy().walletHandler.removeFavoriteApp(appItem)
  const walletInfo = await refreshWalletInfo({
    ...store.getState().wallet,
    favoriteApps: store.getState().wallet.favoriteApps
      .filter((app) =>
        app.description !== appItem.description &&
        app.name !== appItem.name &&
        app.url !== appItem.url
      )
  })
  store.dispatch(WalletActions.walletInfoUpdated(walletInfo))
})

handler.on(WalletActions.chainChangedEvent.getType(), async (store: Store, payload: ChainChangedEventPayloadType) => {
  alert(`h - ${WalletActions.chainChangedEvent.getType()}`)
  console.log(`h - ${WalletActions.chainChangedEvent.getType()}`)
  const { keyringService } = getAPIProxy()
  const selectedAccountAddress = await keyringService.getSelectedAccount(payload.coin)

  const { accounts } = getWalletState(store)
  const selectedAccount = accounts.find((account) => account.address === selectedAccountAddress.address) ?? accounts[0]

  store.dispatch(WalletActions.chainChangeComplete({
    selectedAccount,
    coin: payload.coin
  }))
})

handler.on(WalletActions.selectNetwork.getType(), async (store: Store, payload: BraveWallet.NetworkInfo) => {
  alert(`h - ${WalletActions.selectNetwork.getType()}`)
  console.log(`h - ${WalletActions.selectNetwork.getType()}`)
  await getAPIProxy().jsonRpcService.setNetwork(payload.chainId, payload.coin)
  store.dispatch(WalletActions.setNetwork(payload))
})

handler.on(WalletActions.selectAccount.getType(), async (store: Store, payload: WalletAccountType) => {
  alert(`h - ${WalletActions.selectAccount.getType()}`)
  console.log(`h - ${WalletActions.selectAccount.getType()}`)
  const { keyringService } = getAPIProxy()
  const { defaultNetworks, accounts, transactions } = getWalletState(store)

  const defaultCoinTypesNetwork = defaultNetworks.find((network) => network.coin === payload.coin) ?? defaultNetworks[0]
  await keyringService.setSelectedAccount(payload.address, payload.coin)

  const refreshedTransactionHistory = await refreshTransactionHistory({ accounts, transactions }, payload.address)

  store.dispatch(WalletActions.setAccount({
    coin: payload.coin,
    selectedAccount: payload,
    selectedNetwork: defaultCoinTypesNetwork,
    ...refreshedTransactionHistory
  }))
})

handler.on(WalletActions.getAllNetworks.getType(), async (store) => {
  alert(`h - ${WalletActions.getAllNetworks.getType()}`)
  console.log(`h - ${WalletActions.getAllNetworks.getType()}`)
  const { jsonRpcService } = getAPIProxy()

  const getFullNetworkList = await Promise.all(SupportedCoinTypes.map(async (coin: BraveWallet.CoinType) => {
    const { networks } = await jsonRpcService.getAllNetworks(coin)
    return networks
  }))
  const networkList = getFullNetworkList.flat(1)
  store.dispatch(WalletActions.setAllNetworks(networkList))
})

handler.on(WalletActions.getAllTokensList.getType(), async (store) => {
  alert(`h - ${WalletActions.getAllTokensList.getType()}`)
  console.log(`h - ${WalletActions.getAllTokensList.getType()}`)
  const allTokensList = await getAllTokensList(getWalletState(store).networkList)
  store.dispatch(WalletActions.setAllTokensList(allTokensList))
})

handler.on(WalletActions.addUserAsset.getType(), async (store: Store, payload: BraveWallet.BlockchainToken) => {
  alert(`h - ${WalletActions.addUserAsset.getType()}`)
  console.log(`h - ${WalletActions.addUserAsset.getType()}`)
  const { braveWalletService } = getAPIProxy()
  const result = await braveWalletService.addUserAsset(payload)
  store.dispatch(WalletActions.addUserAssetError(!result.success))
})

handler.on(WalletActions.removeUserAsset.getType(), async (store: Store, payload: BraveWallet.BlockchainToken) => {
  alert(`h - ${WalletActions.removeUserAsset.getType()}`)
  console.log(`h - ${WalletActions.removeUserAsset.getType()}`)
  const { braveWalletService } = getAPIProxy()
  await braveWalletService.removeUserAsset(payload)
})

handler.on(WalletActions.setUserAssetVisible.getType(), async (store: Store, payload: SetUserAssetVisiblePayloadType) => {
  alert(`h - ${WalletActions.setUserAssetVisible.getType()}`)
  console.log(`h - ${WalletActions.setUserAssetVisible.getType()}`)
  const { braveWalletService } = getAPIProxy()
  await braveWalletService.setUserAssetVisible(payload.token, payload.isVisible)
})

handler.on(WalletActions.refreshBalancesAndPriceHistory.getType(), async (store: Store) => {
  alert(`h - ${WalletActions.refreshBalancesAndPriceHistory.getType()}`)
  console.log(`h - ${WalletActions.refreshBalancesAndPriceHistory.getType()}`)
  const payload = await refreshBalancesPricesAndHistory(store.getState().wallet)
  store.dispatch(WalletActions.balancesAndPriceHistoryRefreshed(payload))
})

handler.on(WalletActions.selectPortfolioTimeline.getType(), async (store: Store, payload: BraveWallet.AssetPriceTimeframe) => {
  alert(`h - ${WalletActions.selectPortfolioTimeline.getType()}`)
  console.log(`h - ${WalletActions.selectPortfolioTimeline.getType()}`)
  const portfolioPriceHistory = await refreshPortfolioPriceHistory({
    ...store.getState().wallet,
    selectedPortfolioTimeline: payload
  })
  store.dispatch(WalletActions.portfolioPriceHistoryUpdated(portfolioPriceHistory))
  store.dispatch(WalletActions.portfolioTimelineUpdated(payload))
})

handler.on(WalletActions.sendTransaction.getType(), async (store: Store, payload: SendEthTransactionParams | SendFilTransactionParams | SendSolTransactionParams) => {
  alert(`h - ${WalletActions.sendTransaction.getType()}`)
  console.log(`h - ${WalletActions.sendTransaction.getType()}`)
  let addResult
  if (payload.coin === BraveWallet.CoinType.ETH) {
    addResult = await sendEthTransaction(store, payload as SendEthTransactionParams)
  } else if (payload.coin === BraveWallet.CoinType.FIL) {
    addResult = await sendFilTransaction(payload as SendFilTransactionParams)
  } else if (payload.coin === BraveWallet.CoinType.SOL) {
    addResult = await sendSolTransaction(payload as SendSolTransactionParams)
  }
  if (addResult && !addResult.success) {
    console.log(
      'Sending unapproved transaction failed: ' +
      `from=${payload.from} err=${addResult.errorMessage}`
    )
    return
  }

  // Refresh the transaction history of the origin account.
  const txHistory = await refreshTransactionHistory(
    getWalletState(store),
    payload.from
  )
  store.dispatch(WalletActions.transactionHistoryRefreshed(txHistory))
})

handler.on(WalletActions.sendSPLTransfer.getType(), async (store: Store, payload: SPLTransferFromParams) => {
  alert(`h - ${WalletActions.sendSPLTransfer.getType()}`)
  console.log(`h - ${WalletActions.sendSPLTransfer.getType()}`)
  const { solanaTxManagerProxy } = getAPIProxy()
  const value = await solanaTxManagerProxy.makeTokenProgramTransferTxData(payload.splTokenMintAddress, payload.from, payload.to, BigInt(payload.value))
  if (!value.txData) {
    console.log('Failed making SPL transfer data, to: ', payload.to, ', value: ', payload.value)
    return
  }
  await sendSPLTransaction(value.txData)
  const txHistory = await refreshTransactionHistory(getWalletState(store), payload.from)
  store.dispatch(WalletActions.transactionHistoryRefreshed(txHistory))
})

handler.on(WalletActions.sendERC20Transfer.getType(), async (store: Store, payload: ER20TransferParams) => {
  alert(`h - ${WalletActions.sendERC20Transfer.getType()}`)
  console.log(`h - ${WalletActions.sendERC20Transfer.getType()}`)
  const apiProxy = getAPIProxy()
  const { data, success } = await apiProxy.ethTxManagerProxy.makeERC20TransferData(payload.to, payload.value)
  if (!success) {
    console.log('Failed making ERC20 transfer data, to: ', payload.to, ', value: ', payload.value)
    return
  }

  store.dispatch(WalletActions.sendTransaction({
    from: payload.from,
    to: payload.contractAddress,
    value: '0x0',
    gas: payload.gas,
    gasPrice: payload.gasPrice,
    maxPriorityFeePerGas: payload.maxPriorityFeePerGas,
    maxFeePerGas: payload.maxFeePerGas,
    data,
    coin: BraveWallet.CoinType.ETH
  }))
})

handler.on(WalletActions.sendERC721TransferFrom.getType(), async (store: Store, payload: ERC721TransferFromParams) => {
  alert(`h - ${WalletActions.sendERC721TransferFrom.getType()}`)
  console.log(`h - ${WalletActions.sendERC721TransferFrom.getType()}`)
  const apiProxy = getAPIProxy()
  const { data, success } = await apiProxy.ethTxManagerProxy.makeERC721TransferFromData(payload.from, payload.to, payload.tokenId, payload.contractAddress)
  if (!success) {
    console.log('Failed making ERC721 transferFrom data, from: ', payload.from, ', to: ', payload.to, ', tokenId: ', payload.tokenId)
    return
  }

  store.dispatch(WalletActions.sendTransaction({
    from: payload.from,
    to: payload.contractAddress,
    value: '0x0',
    gas: payload.gas,
    gasPrice: payload.gasPrice,
    maxPriorityFeePerGas: payload.maxPriorityFeePerGas,
    maxFeePerGas: payload.maxFeePerGas,
    data,
    coin: BraveWallet.CoinType.ETH
  }))
})

handler.on(WalletActions.approveERC20Allowance.getType(), async (store: Store, payload: ApproveERC20Params) => {
  alert(`h - ${WalletActions.approveERC20Allowance.getType()}`)
  console.log(`h - ${WalletActions.approveERC20Allowance.getType()}`)
  const apiProxy = getAPIProxy()
  const { data, success } = await apiProxy.ethTxManagerProxy.makeERC20ApproveData(payload.spenderAddress, payload.allowance)
  if (!success) {
    console.log(
      'Failed making ERC20 approve data, contract: ',
      payload.contractAddress,
      ', spender: ', payload.spenderAddress,
      ', allowance: ', payload.allowance
    )
    return
  }

  store.dispatch(WalletActions.sendTransaction({
    from: payload.from,
    to: payload.contractAddress,
    value: '0x0',
    data,
    coin: BraveWallet.CoinType.ETH
  }))
})

handler.on(WalletActions.approveTransaction.getType(), async (store: Store, txInfo: BraveWallet.TransactionInfo) => {
  alert(`h - ${WalletActions.approveTransaction.getType()}`)
  console.log(`h - ${WalletActions.approveTransaction.getType()}`)
  const apiProxy = getAPIProxy()
  const coin = getCoinFromTxDataUnion(txInfo.txDataUnion)
  const result = await apiProxy.txService.approveTransaction(coin, txInfo.id)
  const error = result.errorUnion.providerError ?? result.errorUnion.solanaProviderError
  if (error !== BraveWallet.ProviderError.kSuccess) {
    store.dispatch(WalletActions.setTransactionProviderError({
      transaction: txInfo,
      providerError: {
        code: error,
        message: result.errorMessage
      } as TransactionProviderError
    }))
  }

  const txHistory = await refreshTransactionHistory(getWalletState(store), txInfo.fromAddress)
  store.dispatch(WalletActions.transactionHistoryRefreshed(txHistory))
})

handler.on(WalletActions.rejectTransaction.getType(), async (store: Store, txInfo: BraveWallet.TransactionInfo) => {
  alert(`h - ${WalletActions.rejectTransaction.getType()}`)
  console.log(`h - ${WalletActions.rejectTransaction.getType()}`)
  const { txService } = getAPIProxy()
  const coin = getCoinFromTxDataUnion(txInfo.txDataUnion)
  await txService.rejectTransaction(coin, txInfo.id)
  const txHistory = await refreshTransactionHistory(getWalletState(store), txInfo.fromAddress)
  store.dispatch(WalletActions.transactionHistoryRefreshed(txHistory))
})

handler.on(WalletActions.rejectAllTransactions.getType(), async (store) => {
  alert(`h - ${WalletActions.rejectAllTransactions.getType()}`)
  console.log(`h - ${WalletActions.rejectAllTransactions.getType()}`)
  const state = getWalletState(store)
  const { txService } = getAPIProxy()

  await Promise.all(state.pendingTransactions.map(async (transaction) => {
    const coin = getCoinFromTxDataUnion(transaction.txDataUnion)
    return txService.rejectTransaction(coin, transaction.id)
  }))

  const walletInfo = await refreshWalletInfo(state)
  store.dispatch(WalletActions.walletInfoUpdated(walletInfo))
})

handler.on(WalletActions.refreshGasEstimates.getType(), async (store: Store, txInfo: BraveWallet.TransactionInfo) => {
  alert(`h - ${WalletActions.refreshGasEstimates.getType()}`)
  console.log(`h - ${WalletActions.refreshGasEstimates.getType()}`)
  const { selectedAccount, selectedNetwork } = getWalletState(store)
  const { ethTxManagerProxy, solanaTxManagerProxy } = getAPIProxy()

  if (
    txInfo.txType === BraveWallet.TransactionType.SolanaSystemTransfer ||
    txInfo.txType === BraveWallet.TransactionType.SolanaSPLTokenTransfer ||
    txInfo.txType === BraveWallet.TransactionType.SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
  ) {
    const getSolFee = await solanaTxManagerProxy.getEstimatedTxFee(txInfo.id)
    if (!getSolFee.fee) {
      console.error('Failed to fetch SOL Fee estimates')
      return
    }
    store.dispatch(WalletActions.setSolFeeEstimates({ fee: getSolFee.fee }))
    return
  }

  if (!hasEIP1559Support(selectedAccount, selectedNetwork)) {
    return
  }

  const basicEstimates = await ethTxManagerProxy.getGasEstimation1559()
  if (!basicEstimates.estimation) {
    console.error('Failed to fetch gas estimates')
    return
  }

  store.dispatch(WalletActions.setGasEstimates(basicEstimates.estimation))
})

handler.on(WalletActions.updateUnapprovedTransactionGasFields.getType(), async (store: Store, payload: UpdateUnapprovedTransactionGasFieldsType) => {
  alert(`h - ${WalletActions.updateUnapprovedTransactionGasFields.getType()}`)
  console.log(`h - ${WalletActions.updateUnapprovedTransactionGasFields.getType()}`)
  const apiProxy = getAPIProxy()

  const isEIP1559 = payload.maxPriorityFeePerGas !== undefined && payload.maxFeePerGas !== undefined

  if (isEIP1559) {
    const result = await apiProxy.ethTxManagerProxy.setGasFeeAndLimitForUnapprovedTransaction(
      payload.txMetaId,
      payload.maxPriorityFeePerGas || '',
      payload.maxFeePerGas || '',
      payload.gasLimit
    )

    if (!result.success) {
      console.error(
        'Failed to update unapproved transaction: ' +
        `id=${payload.txMetaId} ` +
        `maxPriorityFeePerGas=${payload.maxPriorityFeePerGas}` +
        `maxFeePerGas=${payload.maxFeePerGas}` +
        `gasLimit=${payload.gasLimit}`
      )
    }
  }

  if (!isEIP1559 && payload.gasPrice) {
    const result = await apiProxy.ethTxManagerProxy.setGasPriceAndLimitForUnapprovedTransaction(
      payload.txMetaId, payload.gasPrice, payload.gasLimit
    )

    if (!result.success) {
      console.error(
        'Failed to update unapproved transaction: ' +
        `id=${payload.txMetaId} ` +
        `gasPrice=${payload.gasPrice}` +
        `gasLimit=${payload.gasLimit}`
      )
    }
  }
})

handler.on(WalletActions.updateUnapprovedTransactionSpendAllowance.getType(), async (store: Store, payload: UpdateUnapprovedTransactionSpendAllowanceType) => {
  alert(`h - ${WalletActions.updateUnapprovedTransactionSpendAllowance.getType()}`)
  console.log(`h - ${WalletActions.updateUnapprovedTransactionSpendAllowance.getType()}`)
  const apiProxy = getAPIProxy()

  const { data, success } = await apiProxy.ethTxManagerProxy.makeERC20ApproveData(payload.spenderAddress, payload.allowance)
  if (!success) {
    console.error(
      `Failed making ERC20 approve data, spender: ${payload.spenderAddress}` +
      `, allowance: ${payload.allowance}`
    )
    return
  }

  const result = await apiProxy.ethTxManagerProxy.setDataForUnapprovedTransaction(payload.txMetaId, data)
  if (!result.success) {
    console.error(
      'Failed to update unapproved transaction: ' +
      `id=${payload.txMetaId} ` +
      `allowance=${payload.allowance}`
    )
  }
})

handler.on(WalletActions.updateUnapprovedTransactionNonce.getType(), async (store: Store, payload: UpdateUnapprovedTransactionNonceType) => {
  alert(`h - ${WalletActions.updateUnapprovedTransactionNonce.getType()}`)
  console.log(`h - ${WalletActions.updateUnapprovedTransactionNonce.getType()}`)
  const { ethTxManagerProxy } = getAPIProxy()

  const result = await ethTxManagerProxy.setNonceForUnapprovedTransaction(payload.txMetaId, payload.nonce)
  if (!result.success) {
    console.error(
      'Failed to update unapproved transaction: ' +
      `id=${payload.txMetaId} ` +
      `nonce=${payload.nonce}`
    )
  }
})

handler.on(WalletActions.removeSitePermission.getType(), async (store: Store, payload: RemoveSitePermissionPayloadType) => {
  alert(`h - ${WalletActions.removeSitePermission.getType()}`)
  console.log(`h - ${WalletActions.removeSitePermission.getType()}`)
  const { braveWalletService } = getAPIProxy()
  await braveWalletService.resetEthereumPermission(payload.origin, payload.account)

  const walletInfo = await refreshWalletInfo(getWalletState(store))
  store.dispatch(WalletActions.walletInfoUpdated(walletInfo))
})

handler.on(WalletActions.addSitePermission.getType(), async (store: Store, payload: AddSitePermissionPayloadType) => {
  alert(`h - ${WalletActions.addSitePermission.getType()}`)
  console.log(`h - ${WalletActions.addSitePermission.getType()}`)
  const braveWalletService = getAPIProxy().braveWalletService
  await braveWalletService.addEthereumPermission(payload.origin, payload.account)

  const walletInfo = await refreshWalletInfo(getWalletState(store))
  store.dispatch(WalletActions.walletInfoUpdated(walletInfo))
})

handler.on(WalletActions.transactionStatusChanged.getType(), async (store: Store, payload: TransactionStatusChanged) => {
  alert(`h - ${WalletActions.transactionStatusChanged.getType()}`)
  console.log(`h - ${WalletActions.transactionStatusChanged.getType()}`)
  const status = payload.txInfo.txStatus
  if (status === BraveWallet.TransactionStatus.Confirmed || status === BraveWallet.TransactionStatus.Error) {
    const payload = await refreshBalancesPricesAndHistory(store.getState().wallet)
    store.dispatch(WalletActions.balancesAndPriceHistoryRefreshed(payload))
  }
})

handler.on(WalletActions.retryTransaction.getType(), async (store: Store, payload: BraveWallet.TransactionInfo) => {
  alert(`h - ${WalletActions.retryTransaction.getType()}`)
  console.log(`h - ${WalletActions.retryTransaction.getType()}`)
  const { txService } = getAPIProxy()
  const coin = getCoinFromTxDataUnion(payload.txDataUnion)
  const result = await txService.retryTransaction(coin, payload.id)
  if (!result.success) {
    console.error(
      'Retry transaction failed: ' +
      `id=${payload.id} ` +
      `err=${result.errorMessage}`
    )
  } else {
    // Refresh the transaction history of the origin account.
    const txHistory = await refreshTransactionHistory(getWalletState(store), payload.fromAddress)
    store.dispatch(WalletActions.transactionHistoryRefreshed(txHistory))
  }
})

handler.on(WalletActions.speedupTransaction.getType(), async (store: Store, payload: BraveWallet.TransactionInfo) => {
  alert(`h - ${WalletActions.speedupTransaction.getType()}`)
  console.log(`h - ${WalletActions.speedupTransaction.getType()}`)
  const { txService } = getAPIProxy()
  const coin = getCoinFromTxDataUnion(payload.txDataUnion)
  const result = await txService.speedupOrCancelTransaction(coin, payload.id, false)
  if (!result.success) {
    console.error(
      'Speedup transaction failed: ' +
      `id=${payload.id} ` +
      `err=${result.errorMessage}`
    )
  } else {
    // Refresh the transaction history of the origin account.
    const txHistory = await refreshTransactionHistory(getWalletState(store), payload.fromAddress)
    store.dispatch(WalletActions.transactionHistoryRefreshed(txHistory))
  }
})

handler.on(WalletActions.cancelTransaction.getType(), async (store: Store, payload: BraveWallet.TransactionInfo) => {
  alert(`h - ${WalletActions.cancelTransaction.getType()}`)
  console.log(`h - ${WalletActions.cancelTransaction.getType()}`)
  const { txService } = getAPIProxy()
  const coin = getCoinFromTxDataUnion(payload.txDataUnion)
  const result = await txService.speedupOrCancelTransaction(coin, payload.id, true)
  if (!result.success) {
    console.error(
      'Cancel transaction failed: ' +
      `id=${payload.id} ` +
      `err=${result.errorMessage}`
    )
  } else {
    // Refresh the transaction history of the origin account.
    const txHistory = await refreshTransactionHistory(getWalletState(store), payload.fromAddress)
    store.dispatch(WalletActions.transactionHistoryRefreshed(txHistory))
  }
})

handler.on(WalletActions.expandWalletNetworks.getType(), async (store) => {
  alert(`h - ${WalletActions.expandWalletNetworks.getType()}`)
  console.log(`h - ${WalletActions.expandWalletNetworks.getType()}`)
  chrome.tabs.create({ url: 'chrome://settings/wallet/networks' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(WalletActions.setSelectedNetworkFilter.getType(), async (store: Store, payload: BraveWallet.NetworkInfo) => {
  alert(`h - ${WalletActions.setSelectedNetworkFilter.getType()}`)
  console.log(`h - ${WalletActions.setSelectedNetworkFilter.getType()}`)
  const portfolioPriceHistory = await refreshPortfolioPriceHistory(getWalletState(store))
  store.dispatch(WalletActions.portfolioPriceHistoryUpdated(portfolioPriceHistory))
})

export default handler.middleware
