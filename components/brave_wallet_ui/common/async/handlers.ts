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
  SelectedAccountChangedPayloadType
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
  SendFilTransactionParams,
  SendSolTransactionParams,
  SPLTransferFromParams
} from '../../constants/types'
import {
  AddAccountPayloadType,
  AddFilecoinAccountPayloadType
} from '../../page/constants/action_types'

// Utils

import getAPIProxy from './bridge'
import {
  hasEIP1559Support,
  refreshKeyringInfo,
  refreshNetworkInfo,
  refreshFullNetworkList,
  refreshTokenPriceHistory,
  refreshSitePermissions,
  refreshTransactionHistory,
  refreshBalances,
  refreshVisibleTokenInfo,
  refreshPrices,
  sendEthTransaction,
  sendFilTransaction,
  sendSolTransaction,
  sendSPLTransaction
} from './lib'
import { Store } from './types'
import InteractionNotifier from './interactionNotifier'
import BalanceUpdater from './balanceUpdater'
import { getCoinFromTxDataUnion, getNetworkInfo } from '../../utils/network-utils'

const handler = new AsyncActionHandler()

const interactionNotifier = new InteractionNotifier()
const balanceUpdater = new BalanceUpdater()

function getWalletState (store: Store): WalletState {
  return store.getState().wallet
}

async function refreshBalancesPricesAndHistory (store: Store) {
  const state = getWalletState(store)
  await store.dispatch(refreshVisibleTokenInfo(state.selectedNetwork))
  await store.dispatch(refreshBalances())
  await store.dispatch(refreshPrices())
  await store.dispatch(refreshTokenPriceHistory(state.selectedPortfolioTimeline))
}

async function refreshWalletInfo (store: Store) {
  const apiProxy = getAPIProxy()

  const selectedCoin = await apiProxy.braveWalletService.getSelectedCoin()
  store.dispatch(WalletActions.setSelectedCoin(selectedCoin.coin))

  await store.dispatch(refreshKeyringInfo())
  await store.dispatch(refreshNetworkInfo())

  // Populate tokens from blockchain registry.
  store.dispatch(WalletActions.getAllTokensList())

  const braveWalletService = apiProxy.braveWalletService
  const defaultEthereumResult = await braveWalletService.getDefaultEthereumWallet()
  store.dispatch(WalletActions.defaultEthereumWalletUpdated(defaultEthereumResult.defaultWallet))
  const defaultSolanaResult = await braveWalletService.getDefaultSolanaWallet()
  store.dispatch(WalletActions.defaultSolanaWalletUpdated(defaultSolanaResult.defaultWallet))

  const mmResult =
    await braveWalletService.isExternalWalletInstalled(
      BraveWallet.ExternalWalletType.MetaMask)
  store.dispatch(WalletActions.setMetaMaskInstalled(mmResult.installed))

  await store.dispatch(refreshTransactionHistory())
  await store.dispatch(refreshSitePermissions())
  store.dispatch(WalletActions.getOnRampCurrencies())
}

async function updateAccountInfo (store: Store) {
  const state = getWalletState(store)
  const apiProxy = getAPIProxy()
  const { walletHandler } = apiProxy
  const walletInfo = await walletHandler.getWalletInfo()
  if (state.accounts.length === walletInfo.accountInfos.length) {
    await store.dispatch(WalletActions.refreshAccountInfo(walletInfo))
  } else {
    await refreshWalletInfo(store)
  }
}

async function updateCoinAccountNetworkInfo (store: Store, coin: BraveWallet.CoinType) {
  const { accounts, networkList } = getWalletState(store)
  if (accounts.length === 0) {
    return
  }
  const { braveWalletService, keyringService, jsonRpcService } = getAPIProxy()
  const coinsChainId = await jsonRpcService.getChainId(coin)

  // Update Selected Coin
  await braveWalletService.setSelectedCoin(coin)
  await store.dispatch(WalletActions.setSelectedCoin(coin))

  // Updated Selected Account
  const selectedAccountAddress = coin === BraveWallet.CoinType.FIL
      ? await keyringService.getFilecoinSelectedAccount(coinsChainId.chainId)
      : await keyringService.getSelectedAccount(coin)
  const defaultAccount = accounts.find((account) => account.address === selectedAccountAddress.address) || accounts[0]
  await store.dispatch(WalletActions.setSelectedAccount(defaultAccount))
  await store.dispatch(refreshTransactionHistory(defaultAccount.address))

  // Updated Selected Network
  const defaultNetwork = getNetworkInfo(coinsChainId.chainId, coin, networkList)
  await store.dispatch(WalletActions.setNetwork(defaultNetwork))
}

handler.on(WalletActions.refreshBalancesAndPrices.getType(), async (store: Store) => {
  const state = getWalletState(store)
  await store.dispatch(refreshVisibleTokenInfo(state.selectedNetwork))
  await store.dispatch(refreshBalances())
  await store.dispatch(refreshPrices())
})

handler.on(WalletActions.initialize.getType(), async (store) => {
  // Initialize active origin state.
  const braveWalletService = getAPIProxy().braveWalletService
  const { originInfo } = await braveWalletService.getActiveOrigin()
  store.dispatch(WalletActions.activeOriginChanged(originInfo))
  await refreshWalletInfo(store)
})

handler.on(WalletActions.keyringCreated.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.keyringRestored.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.keyringReset.getType(), async (store) => {
  window.location.reload()
})

handler.on(WalletActions.locked.getType(), async (store) => {
  interactionNotifier.stopWatchingForInteraction()
  balanceUpdater.stopUpdatingBalances()
  await refreshWalletInfo(store)
})

handler.on(WalletActions.unlocked.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.backedUp.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.accountsChanged.getType(), async (store) => {
  await updateAccountInfo(store)
})

handler.on(WalletActions.selectNetwork.getType(), async (store: Store, payload: BraveWallet.NetworkInfo) => {
  const { jsonRpcService } = getAPIProxy()
  await jsonRpcService.setNetwork(payload.chainId, payload.coin)
})

handler.on(WalletActions.chainChangedEvent.getType(), async (store: Store, payload: ChainChangedEventPayloadType) => {
  await updateCoinAccountNetworkInfo(store, payload.coin)
})

handler.on(WalletActions.selectAccount.getType(), async (store: Store, payload: WalletAccountType) => {
  const { keyringService } = getAPIProxy()
  await keyringService.setSelectedAccount(payload.address, payload.coin)
})

handler.on(WalletActions.selectedAccountChanged.getType(), async (store, payload: SelectedAccountChangedPayloadType) => {
  await updateCoinAccountNetworkInfo(store, payload.coin)
})

handler.on(WalletActions.defaultEthereumWalletChanged.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.defaultSolanaWalletChanged.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.defaultBaseCurrencyChanged.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.defaultBaseCryptocurrencyChanged.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.lockWallet.getType(), async (store) => {
  const keyringService = getAPIProxy().keyringService
  keyringService.lock()
})

handler.on(WalletActions.unlockWallet.getType(), async (store: Store, payload: UnlockWalletPayloadType) => {
  const keyringService = getAPIProxy().keyringService
  const result = await keyringService.unlock(payload.password)
  store.dispatch(WalletActions.hasIncorrectPassword(!result.success))
})

handler.on(WalletActions.addFavoriteApp.getType(), async (store: Store, appItem: BraveWallet.AppItem) => {
  const walletHandler = getAPIProxy().walletHandler
  walletHandler.addFavoriteApp(appItem)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.removeFavoriteApp.getType(), async (store: Store, appItem: BraveWallet.AppItem) => {
  const walletHandler = getAPIProxy().walletHandler
  walletHandler.removeFavoriteApp(appItem)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.initialized.getType(), async (store: Store, payload: WalletInfo) => {
  const keyringService = getAPIProxy().keyringService
  const state = getWalletState(store)
  if (!state.isWalletLocked) {
    keyringService.notifyUserInteraction()
  }
  interactionNotifier.beginWatchingForInteraction(50000, state.isWalletLocked, async () => {
    keyringService.notifyUserInteraction()
  })
  const braveWalletService = getAPIProxy().braveWalletService
  const defaultFiat = await braveWalletService.getDefaultBaseCurrency()
  const defualtCrypo = await braveWalletService.getDefaultBaseCryptocurrency()
  const defaultCurrencies = {
    fiat: defaultFiat.currency,
    crypto: defualtCrypo.cryptocurrency
  }
  store.dispatch(WalletActions.defaultCurrenciesUpdated(defaultCurrencies))
  // Fetch Balances and Prices
  if (!state.isWalletLocked && state.isWalletCreated) {
    const currentNetwork = await store.dispatch(refreshNetworkInfo())
    await store.dispatch(refreshVisibleTokenInfo(currentNetwork))
    await store.dispatch(refreshBalances())
    balanceUpdater.beginUpdatingBalances(15000, async () => {
      await store.dispatch(refreshBalances())
    })
    await store.dispatch(refreshPrices())
    await store.dispatch(refreshTokenPriceHistory(state.selectedPortfolioTimeline))
  }

  // This can be 0 when the wallet is locked
  if (payload.selectedAccount) {
    await store.dispatch(refreshTransactionHistory(payload.selectedAccount))
  }
})

handler.on(WalletActions.getAllNetworks.getType(), async (store: Store) => {
  await store.dispatch(refreshFullNetworkList())
})

handler.on(WalletActions.getAllTokensList.getType(), async (store) => {
  const state = getWalletState(store)
  const { networkList } = state
  const { blockchainRegistry } = getAPIProxy()
  const getAllTokensList = await Promise.all(networkList.map(async (network) => {
    const list = await blockchainRegistry.getAllTokens(network.chainId, network.coin)
    return list.tokens.map((token) => {
      return {
        ...token,
        chainId: network.chainId,
        logo: `chrome://erc-token-images/${token.logo}`
      }
    })
  }))
  const allTokensList = getAllTokensList.flat(1)
  store.dispatch(WalletActions.setAllTokensList(allTokensList))
})

handler.on(WalletActions.addUserAsset.getType(), async (store: Store, payload: BraveWallet.BlockchainToken) => {
  const { braveWalletService } = getAPIProxy()
  const result = await braveWalletService.addUserAsset(payload)
  store.dispatch(WalletActions.addUserAssetError(!result.success))
})

handler.on(WalletActions.removeUserAsset.getType(), async (store: Store, payload: BraveWallet.BlockchainToken) => {
  const { braveWalletService } = getAPIProxy()
  await braveWalletService.removeUserAsset(payload)
})

handler.on(WalletActions.setUserAssetVisible.getType(), async (store: Store, payload: SetUserAssetVisiblePayloadType) => {
  const { braveWalletService } = getAPIProxy()
  await braveWalletService.setUserAssetVisible(payload.token, payload.isVisible)
})

handler.on(WalletActions.refreshBalancesAndPriceHistory.getType(), async (store: Store) => {
  refreshBalancesPricesAndHistory(store)
})

handler.on(WalletActions.selectPortfolioTimeline.getType(), async (store: Store, payload: BraveWallet.AssetPriceTimeframe) => {
  store.dispatch(WalletActions.portfolioTimelineUpdated(payload))
  await store.dispatch(refreshTokenPriceHistory(payload))
})

handler.on(WalletActions.sendTransaction.getType(), async (
  store: Store,
  payload: SendEthTransactionParams | SendFilTransactionParams | SendSolTransactionParams
) => {
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
  await store.dispatch(refreshTransactionHistory(payload.from))
})

handler.on(WalletActions.sendSPLTransfer.getType(), async (store: Store, payload: SPLTransferFromParams) => {
  const { solanaTxManagerProxy } = getAPIProxy()
  const value = await solanaTxManagerProxy.makeTokenProgramTransferTxData(payload.splTokenMintAddress, payload.from, payload.to, BigInt(payload.value))
  if (!value.txData) {
    console.log('Failed making SPL transfer data, to: ', payload.to, ', value: ', payload.value)
    return
  }
  await sendSPLTransaction(value.txData)
  await store.dispatch(refreshTransactionHistory(payload.from))
})

handler.on(WalletActions.sendERC20Transfer.getType(), async (store: Store, payload: ER20TransferParams) => {
  const apiProxy = getAPIProxy()
  const { data, success } = await apiProxy.ethTxManagerProxy.makeERC20TransferData(payload.to, payload.value)
  if (!success) {
    console.log('Failed making ERC20 transfer data, to: ', payload.to, ', value: ', payload.value)
    return
  }

  await store.dispatch(WalletActions.sendTransaction({
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
  const apiProxy = getAPIProxy()
  const { data, success } = await apiProxy.ethTxManagerProxy.makeERC721TransferFromData(payload.from, payload.to, payload.tokenId, payload.contractAddress)
  if (!success) {
    console.log('Failed making ERC721 transferFrom data, from: ', payload.from, ', to: ', payload.to, ', tokenId: ', payload.tokenId)
    return
  }

  await store.dispatch(WalletActions.sendTransaction({
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

  await store.dispatch(WalletActions.sendTransaction({
    from: payload.from,
    to: payload.contractAddress,
    value: '0x0',
    data,
    coin: BraveWallet.CoinType.ETH
  }))
})

handler.on(WalletActions.approveTransaction.getType(), async (store: Store, txInfo: BraveWallet.TransactionInfo) => {
  const apiProxy = getAPIProxy()
  const coin = getCoinFromTxDataUnion(txInfo.txDataUnion)
  const result = await apiProxy.txService.approveTransaction(coin, txInfo.id)
  const error = result.errorUnion.providerError ?? result.errorUnion.solanaProviderError
  if (error !== BraveWallet.ProviderError.kSuccess) {
    await store.dispatch(WalletActions.setTransactionProviderError({
      transaction: txInfo,
      providerError: {
        code: error,
        message: result.errorMessage
      } as TransactionProviderError
    }))
  }

  await store.dispatch(refreshTransactionHistory(txInfo.fromAddress))
})

handler.on(WalletActions.rejectTransaction.getType(), async (store: Store, txInfo: BraveWallet.TransactionInfo) => {
  const apiProxy = getAPIProxy()
  const coin = getCoinFromTxDataUnion(txInfo.txDataUnion)
  await apiProxy.txService.rejectTransaction(coin, txInfo.id)
  await store.dispatch(refreshTransactionHistory(txInfo.fromAddress))
})

handler.on(WalletActions.rejectAllTransactions.getType(), async (store) => {
  const state = getWalletState(store)
  const apiProxy = getAPIProxy()
  state.pendingTransactions.forEach(async (transaction) => {
    const coin = getCoinFromTxDataUnion(transaction.txDataUnion)
    await apiProxy.txService.rejectTransaction(coin, transaction.id)
  })
  await refreshWalletInfo(store)
})

handler.on(WalletActions.refreshGasEstimates.getType(), async (store: Store, txInfo: BraveWallet.TransactionInfo) => {
  const { selectedAccount, selectedNetwork } = getWalletState(store)
  const { ethTxManagerProxy, solanaTxManagerProxy } = getAPIProxy()

  if (
    txInfo.txType === BraveWallet.TransactionType.SolanaDappSignAndSendTransaction ||
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
  const braveWalletService = getAPIProxy().braveWalletService
  await braveWalletService.resetPermission(payload.coin, payload.origin, payload.account)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.addSitePermission.getType(), async (store: Store, payload: AddSitePermissionPayloadType) => {
  const braveWalletService = getAPIProxy().braveWalletService
  await braveWalletService.addPermission(payload.coin, payload.origin, payload.account)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.transactionStatusChanged.getType(), async (store: Store, payload: TransactionStatusChanged) => {
  const status = payload.txInfo.txStatus
  if (status === BraveWallet.TransactionStatus.Confirmed || status === BraveWallet.TransactionStatus.Error) {
    await refreshBalancesPricesAndHistory(store)
  }
})

handler.on(WalletActions.retryTransaction.getType(), async (store: Store, payload: BraveWallet.TransactionInfo) => {
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
    await store.dispatch(refreshTransactionHistory(payload.fromAddress))
  }
})

handler.on(WalletActions.speedupTransaction.getType(), async (store: Store, payload: BraveWallet.TransactionInfo) => {
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
    await store.dispatch(refreshTransactionHistory(payload.fromAddress))
  }
})

handler.on(WalletActions.cancelTransaction.getType(), async (store: Store, payload: BraveWallet.TransactionInfo) => {
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
    await store.dispatch(refreshTransactionHistory(payload.fromAddress))
  }
})

handler.on(WalletActions.expandWalletNetworks.getType(), async (store) => {
  chrome.tabs.create({ url: 'chrome://settings/wallet/networks' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(WalletActions.setSelectedNetworkFilter.getType(), async (store: Store, payload: BraveWallet.NetworkInfo) => {
  const state = getWalletState(store)
  const { selectedPortfolioTimeline } = state
  await store.dispatch(refreshTokenPriceHistory(selectedPortfolioTimeline))
})

handler.on(WalletActions.addAccount.getType(), async (_store: Store, payload: AddAccountPayloadType) => {
  const { keyringService } = getAPIProxy()
  const result = await keyringService.addAccount(payload.accountName, payload.coin)
  return result.success
})

handler.on(WalletActions.addFilecoinAccount.getType(), async (_store: Store, payload: AddFilecoinAccountPayloadType) => {
  const { keyringService } = getAPIProxy()
  const result = await keyringService.addFilecoinAccount(payload.accountName, payload.network)
  return result.success
})

handler.on(WalletActions.getOnRampCurrencies.getType(), async (store: Store) => {
  const { blockchainRegistry } = getAPIProxy()
  const currencies = (await blockchainRegistry.getOnRampCurrencies()).currencies
  await store.dispatch(WalletActions.setOnRampCurrencies(currencies))
})

export default handler.middleware
