// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { SimpleActionCreator } from 'redux-act'
import BigNumber from 'bignumber.js'

import AsyncActionHandler from '../../../common/AsyncActionHandler'
import * as WalletActions from '../actions/wallet_actions'
import {
  AddUserAssetPayloadType,
  ChainChangedEventPayloadType,
  RemoveSitePermissionPayloadType,
  RemoveUserAssetPayloadType,
  SetUserAssetVisiblePayloadType,
  SwapParamsPayloadType,
  UnlockWalletPayloadType,
  UpdateUnapprovedTransactionGasFieldsType,
  UpdateUnapprovedTransactionSpendAllowanceType,
  TransactionStatusChanged,
  UpdateUnapprovedTransactionNonceType
} from '../constants/action_types'
import {
  BraveWallet,
  ApproveERC20Params,
  ER20TransferParams,
  ERC721TransferFromParams,
  SendTransactionParams,
  SwapErrorResponse,
  WalletAccountType,
  WalletState,
  WalletInfo
} from '../../constants/types'

// Utils
import { hexStrToNumberArray } from '../../utils/hex-utils'
import Amount from '../../utils/amount'

import getAPIProxy from './bridge'
import {
  refreshKeyringInfo,
  refreshNetworkInfo,
  refreshTokenPriceHistory,
  refreshSitePermissions,
  refreshTransactionHistory,
  refreshBalances,
  refreshVisibleTokenInfo,
  refreshPrices,
  sendEthTransaction,
  sendFilTransaction
} from './lib'
import { Store } from './types'
import InteractionNotifier from './interactionNotifier'
import BalanceUpdater from './balanceUpdater'

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

  await store.dispatch(refreshKeyringInfo())
  await store.dispatch(refreshNetworkInfo())

  // Populate tokens from blockchain registry.
  store.dispatch(WalletActions.getAllTokensList())

  const braveWalletService = apiProxy.braveWalletService
  const defaultWallet = await braveWalletService.getDefaultWallet()
  store.dispatch(WalletActions.defaultWalletUpdated(defaultWallet.defaultWallet))

  const mmResult =
    await braveWalletService.isExternalWalletInstalled(
      BraveWallet.ExternalWalletType.MetaMask)
  store.dispatch(WalletActions.setMetaMaskInstalled(mmResult.installed))

  await store.dispatch(refreshTransactionHistory())
  await store.dispatch(refreshSitePermissions())
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

handler.on(WalletActions.refreshBalancesAndPrices.getType(), async (store: Store) => {
  const state = getWalletState(store)
  await store.dispatch(refreshVisibleTokenInfo(state.selectedNetwork))
  await store.dispatch(refreshBalances())
  await store.dispatch(refreshPrices())
})

handler.on(WalletActions.initialize.getType(), async (store) => {
  // Initialize active origin state.
  const braveWalletService = getAPIProxy().braveWalletService
  const origin = await braveWalletService.getActiveOrigin()
  store.dispatch(WalletActions.activeOriginChanged(origin))
  await refreshWalletInfo(store)
})

handler.on(WalletActions.chainChangedEvent.getType(), async (store: Store, payload: ChainChangedEventPayloadType) => {
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

handler.on(WalletActions.selectedAccountChanged.getType(), async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.defaultWalletChanged.getType(), async (store) => {
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

handler.on(WalletActions.selectNetwork.getType(), async (store: Store, payload: BraveWallet.NetworkInfo) => {
  const jsonRpcService = getAPIProxy().jsonRpcService
  await jsonRpcService.setNetwork(payload.chainId, BraveWallet.CoinType.ETH)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.selectAccount.getType(), async (store: Store, payload: WalletAccountType) => {
  const { keyringService } = getAPIProxy()

  await keyringService.setSelectedAccount(payload.address, payload.coin)
  store.dispatch(WalletActions.setSelectedAccount(payload))
  await store.dispatch(refreshTransactionHistory(payload.address))
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

handler.on(WalletActions.getAllNetworks.getType(), async (store) => {
  const jsonRpcService = getAPIProxy().jsonRpcService
  const fullList = await jsonRpcService.getAllNetworks(BraveWallet.CoinType.ETH)
  store.dispatch(WalletActions.setAllNetworks(fullList))
})

handler.on(WalletActions.getAllTokensList.getType(), async (store) => {
  const { blockchainRegistry, jsonRpcService } = getAPIProxy()
  const { chainId } = await jsonRpcService.getChainId(BraveWallet.CoinType.ETH)
  const fullList = await blockchainRegistry.getAllTokens(chainId)
  store.dispatch(WalletActions.setAllTokensList(fullList))
})

handler.on(WalletActions.addUserAsset.getType(), async (store: Store, payload: AddUserAssetPayloadType) => {
  const braveWalletService = getAPIProxy().braveWalletService
  const result = await braveWalletService.addUserAsset(payload.token, payload.chainId)
  store.dispatch(WalletActions.addUserAssetError(!result.success))
})

handler.on(WalletActions.removeUserAsset.getType(), async (store: Store, payload: RemoveUserAssetPayloadType) => {
  const braveWalletService = getAPIProxy().braveWalletService
  await braveWalletService.removeUserAsset(payload.token, payload.chainId)
})

handler.on(WalletActions.setUserAssetVisible.getType(), async (store: Store, payload: SetUserAssetVisiblePayloadType) => {
  const braveWalletService = getAPIProxy().braveWalletService
  await braveWalletService.setUserAssetVisible(payload.token, payload.chainId, payload.isVisible)
})

handler.on(WalletActions.refreshBalancesAndPriceHistory.getType(), async (store: Store) => {
  refreshBalancesPricesAndHistory(store)
})

handler.on(WalletActions.selectPortfolioTimeline.getType(), async (store: Store, payload: BraveWallet.AssetPriceTimeframe) => {
  store.dispatch(WalletActions.portfolioTimelineUpdated(payload))
  await store.dispatch(refreshTokenPriceHistory(payload))
})

handler.on(WalletActions.sendTransaction.getType(), async (store: Store, payload: SendTransactionParams) => {
  let addResult
  if (payload.coin === BraveWallet.CoinType.FIL) {
    addResult = await sendFilTransaction(payload)
  } else {
    addResult = await sendEthTransaction(store, payload)
  }
  if (!addResult.success) {
    console.log(
      'Sending unapproved transaction failed: ' +
      `from=${payload.from} err=${addResult.errorMessage}`
    )
    return
  }
  // Refresh the transaction history of the origin account.
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
  const result = await apiProxy.txService.approveTransaction(BraveWallet.CoinType.ETH, txInfo.id)
  if (result.error !== BraveWallet.ProviderError.kSuccess) {
    console.error(`Failed to approve transaction: ${result.errorMessage}`)
  }

  await store.dispatch(refreshTransactionHistory(txInfo.fromAddress))
})

handler.on(WalletActions.rejectTransaction.getType(), async (store: Store, txInfo: BraveWallet.TransactionInfo) => {
  const apiProxy = getAPIProxy()
  await apiProxy.txService.rejectTransaction(BraveWallet.CoinType.ETH, txInfo.id)
  await store.dispatch(refreshTransactionHistory(txInfo.fromAddress))
})

handler.on(WalletActions.rejectAllTransactions.getType(), async (store) => {
  const state = getWalletState(store)
  const apiProxy = getAPIProxy()
  state.pendingTransactions.forEach(async (transaction) => {
    await apiProxy.txService.rejectTransaction(BraveWallet.CoinType.ETH, transaction.id)
  })
  await refreshWalletInfo(store)
})

// fetchSwapQuoteFactory creates a handler function that can be used with
// both panel and page actions.
export const fetchSwapQuoteFactory = (
  setSwapQuote: SimpleActionCreator<BraveWallet.SwapResponse>,
  setSwapError: SimpleActionCreator<SwapErrorResponse | undefined>
) => async (store: Store, payload: SwapParamsPayloadType) => {
  const { swapService, ethTxManagerProxy } = getAPIProxy()

  const {
    fromAsset,
    fromAssetAmount,
    toAsset,
    toAssetAmount,
    accountAddress,
    slippageTolerance,
    full
  } = payload

  const swapParams = {
    takerAddress: accountAddress,
    sellAmount: fromAssetAmount || '',
    buyAmount: toAssetAmount || '',
    buyToken: toAsset.contractAddress || toAsset.symbol,
    sellToken: fromAsset.contractAddress || fromAsset.symbol,
    slippagePercentage: slippageTolerance.slippage / 100,
    gasPrice: ''
  }

  const quote = await (
    full ? swapService.getTransactionPayload(swapParams) : swapService.getPriceQuote(swapParams)
  )

  if (quote.success && quote.response) {
    await store.dispatch(setSwapError(undefined))
    await store.dispatch(setSwapQuote(quote.response))

    if (full) {
      const {
        to,
        data,
        value,
        estimatedGas
      } = quote.response

      // Get the latest gas estimates, since we'll force the fastest fees in
      // order to ensure a swap with minimum slippage.
      const { estimation: gasEstimates } = await ethTxManagerProxy.getGasEstimation1559()

      let maxPriorityFeePerGas
      let maxFeePerGas
      if (gasEstimates && gasEstimates.fastMaxPriorityFeePerGas === gasEstimates.avgMaxPriorityFeePerGas) {
        // Bump fast priority fee and max fee by 1 GWei if same as average fees.
        const maxPriorityFeePerGasBN = new BigNumber(gasEstimates.fastMaxPriorityFeePerGas).plus(10 ** 9)
        const maxFeePerGasBN = new BigNumber(gasEstimates.fastMaxFeePerGas).plus(10 ** 9)

        maxPriorityFeePerGas = `0x${maxPriorityFeePerGasBN.toString(16)}`
        maxFeePerGas = `0x${maxFeePerGasBN.toString(16)}`
      } else if (gasEstimates) {
        // Always suggest fast gas fees as default
        maxPriorityFeePerGas = gasEstimates.fastMaxPriorityFeePerGas
        maxFeePerGas = gasEstimates.fastMaxFeePerGas
      }

      const params = {
        from: accountAddress,
        to,
        value: new Amount(value).toHex(),
        gas: new Amount(estimatedGas).toHex(),
        data: hexStrToNumberArray(data),
        maxPriorityFeePerGas,
        maxFeePerGas,
        coin: BraveWallet.CoinType.ETH
      }

      store.dispatch(WalletActions.sendTransaction(params))
    }
  } else if (quote.errorResponse) {
    try {
      const err = JSON.parse(quote.errorResponse) as SwapErrorResponse
      await store.dispatch(setSwapError(err))
    } catch (e) {
      console.error(`[swap] error parsing response: ${e}`)
    } finally {
      console.error(`[swap] error querying 0x API: ${quote.errorResponse}`)
    }
  }
}

handler.on(WalletActions.refreshGasEstimates.getType(), async (store) => {
  const ethTxManagerProxy = getAPIProxy().ethTxManagerProxy
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
  await braveWalletService.resetEthereumPermission(payload.origin, payload.account)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.addSitePermission.getType(), async (store: Store, payload: RemoveSitePermissionPayloadType) => {
  const braveWalletService = getAPIProxy().braveWalletService
  await braveWalletService.addEthereumPermission(payload.origin, payload.account)
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
  const result = await txService.retryTransaction(BraveWallet.CoinType.ETH, payload.id)
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
  const result = await txService.speedupOrCancelTransaction(BraveWallet.CoinType.ETH, payload.id, false)
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
  const result = await txService.speedupOrCancelTransaction(BraveWallet.CoinType.ETH, payload.id, true)
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

export default handler.middleware
