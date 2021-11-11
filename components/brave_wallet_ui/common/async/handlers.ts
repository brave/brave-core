// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import { SimpleActionCreator } from 'redux-act'

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
  // DefaultBaseCurrencyChanged,
  // DefaultBaseCryptocurrencyChanged,
  UpdateUnapprovedTransactionGasFieldsType,
  UpdateUnapprovedTransactionSpendAllowanceType,
  TransactionStatusChanged
} from '../constants/action_types'
import {
  AppItem,
  ApproveERC20Params,
  AssetPriceTimeframe,
  ER20TransferParams,
  ERC721TransferFromParams,
  EthereumChain,
  SendTransactionParams,
  SwapErrorResponse,
  SwapResponse,
  TransactionInfo,
  TxData,
  TxData1559,
  WalletAccountType,
  WalletState,
  WalletInfo,
  TransactionStatus
} from '../../constants/types'
import { toWeiHex } from '../../utils/format-balances'
import { hexStrToNumberArray } from '../../utils/hex-utils'
import getAPIProxy from './bridge'
import {
  findHardwareAccountInfo,
  refreshKeyringInfo,
  refreshNetworkInfo,
  refreshTokenPriceHistory,
  refreshSitePermissions,
  refreshTransactionHistory,
  refreshBalancesAndPrices
} from './lib'
import { Store } from './types'
import InteractionNotifier from './interactionNotifier'

import LedgerBridgeKeyring from '../../common/ledgerjs/eth_ledger_bridge_keyring'
import TrezorBridgeKeyring from '../../common/trezor/trezor_bridge_keyring'

const handler = new AsyncActionHandler()

const interactionNotifier = new InteractionNotifier()

function getWalletState (store: Store): WalletState {
  return store.getState().wallet
}

async function refreshBalancesPricesAndHistory (store: Store) {
  const state = getWalletState(store)
  await store.dispatch(refreshBalancesAndPrices(state.selectedNetwork))
  await store.dispatch(refreshTokenPriceHistory(state.selectedPortfolioTimeline))
}

async function refreshWalletInfo (store: Store) {
  const apiProxy = getAPIProxy()
  const state = getWalletState(store)

  await store.dispatch(refreshKeyringInfo())
  const currentNetwork = await store.dispatch(refreshNetworkInfo())

  // Populate tokens from ERC-20 token registry.
  if (state.fullTokenList.length === 0) {
    store.dispatch(WalletActions.getAllTokensList())
  }

  const braveWalletService = apiProxy.braveWalletService
  const defaultWallet = await braveWalletService.getDefaultWallet()
  store.dispatch(WalletActions.defaultWalletUpdated(defaultWallet.defaultWallet))

  const mmResult = await braveWalletService.isMetaMaskInstalled()
  store.dispatch(WalletActions.setMetaMaskInstalled(mmResult.installed))

  await store.dispatch(refreshBalancesAndPrices(currentNetwork))
  await store.dispatch(refreshTokenPriceHistory(state.selectedPortfolioTimeline))
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

  await store.dispatch(refreshBalancesAndPrices(state.selectedNetwork))
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

handler.on(WalletActions.locked.getType(), async (store) => {
  interactionNotifier.stopWatchingForInteraction()
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

// handler.on(WalletActions.defaultBaseCurrencyChanged.getType(), async (store, payload: DefaultBaseCurrencyChanged) => {})
// handler.on(WalletActions.defaultBaseCryptocurrencyChanged.getType(), async (store, payload: DefaultBaseCryptocurrencyChanged) => {})

handler.on(WalletActions.lockWallet.getType(), async (store) => {
  const keyringController = getAPIProxy().keyringController
  keyringController.lock()
})

handler.on(WalletActions.unlockWallet.getType(), async (store: Store, payload: UnlockWalletPayloadType) => {
  const keyringController = getAPIProxy().keyringController
  const result = await keyringController.unlock(payload.password)
  store.dispatch(WalletActions.hasIncorrectPassword(!result.success))
})

handler.on(WalletActions.addFavoriteApp.getType(), async (store: Store, appItem: AppItem) => {
  const walletHandler = getAPIProxy().walletHandler
  walletHandler.addFavoriteApp(appItem)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.removeFavoriteApp.getType(), async (store: Store, appItem: AppItem) => {
  const walletHandler = getAPIProxy().walletHandler
  walletHandler.removeFavoriteApp(appItem)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.selectNetwork.getType(), async (store: Store, payload: EthereumChain) => {
  const ethJsonRpcController = getAPIProxy().ethJsonRpcController
  await ethJsonRpcController.setNetwork(payload.chainId)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.selectAccount.getType(), async (store: Store, payload: WalletAccountType) => {
  const { keyringController } = getAPIProxy()

  await keyringController.setSelectedAccount(payload.address)
  store.dispatch(WalletActions.setSelectedAccount(payload))
  await store.dispatch(refreshTransactionHistory(payload.address))
})

handler.on(WalletActions.initialized.getType(), async (store: Store, payload: WalletInfo) => {
  const isWalletLocked = getWalletState(store).isWalletLocked
  interactionNotifier.beginWatchingForInteraction(50000, isWalletLocked, async () => {
    const keyringController = getAPIProxy().keyringController
    keyringController.notifyUserInteraction()
  })
  // This can be 0 when the wallet is locked
  if (payload.selectedAccount) {
    await store.dispatch(refreshTransactionHistory(payload.selectedAccount))
  }
})

handler.on(WalletActions.getAllNetworks.getType(), async (store) => {
  const ethJsonRpcController = getAPIProxy().ethJsonRpcController
  const fullList = await ethJsonRpcController.getAllNetworks()
  store.dispatch(WalletActions.setAllNetworks(fullList))
})

handler.on(WalletActions.getAllTokensList.getType(), async (store) => {
  const ercTokenRegistry = getAPIProxy().ercTokenRegistry
  const fullList = await ercTokenRegistry.getAllTokens()
  store.dispatch(WalletActions.setAllTokensList(fullList))
})

handler.on(WalletActions.addUserAsset.getType(), async (store: Store, payload: AddUserAssetPayloadType) => {
  const braveWalletService = getAPIProxy().braveWalletService
  const result = await braveWalletService.addUserAsset(payload.token, payload.chainId)
  store.dispatch(WalletActions.addUserAssetError(!result.success))
  await refreshBalancesPricesAndHistory(store)
})

handler.on(WalletActions.removeUserAsset.getType(), async (store: Store, payload: RemoveUserAssetPayloadType) => {
  const braveWalletService = getAPIProxy().braveWalletService
  await braveWalletService.removeUserAsset(payload.token, payload.chainId)
  await refreshBalancesPricesAndHistory(store)
})

handler.on(WalletActions.setUserAssetVisible.getType(), async (store: Store, payload: SetUserAssetVisiblePayloadType) => {
  const braveWalletService = getAPIProxy().braveWalletService
  await braveWalletService.setUserAssetVisible(payload.token, payload.chainId, payload.isVisible)
  await refreshBalancesPricesAndHistory(store)
})

handler.on(WalletActions.selectPortfolioTimeline.getType(), async (store: Store, payload: AssetPriceTimeframe) => {
  store.dispatch(WalletActions.portfolioTimelineUpdated(payload))
  await store.dispatch(refreshTokenPriceHistory(payload))
})

handler.on(WalletActions.sendTransaction.getType(), async (store: Store, payload: SendTransactionParams) => {
  const apiProxy = getAPIProxy()
  /***
   * Determine whether to create a legacy or EIP-1559 transaction.
   *
   * isEIP1559 is true IFF:
   *   - network supports EIP-1559
   *   - keyring supports EIP-1559 (ex: certain hardware wallets vendors)
   *   - payload: SendTransactionParams has specified EIP-1559 gas-pricing
   *     fields.
   *
   * In all other cases, fallback to legacy gas-pricing fields.
   */
  let isEIP1559
  switch (true) {
    // Transaction payload has hardcoded EIP-1559 gas fields.
    case payload.maxPriorityFeePerGas !== undefined && payload.maxFeePerGas !== undefined:
      isEIP1559 = true
      break

    // Transaction payload has hardcoded legacy gas fields.
    case payload.gasPrice !== undefined:
      isEIP1559 = false
      break

    // Check if network and keyring support EIP-1559.
    default:
      const { selectedAccount, selectedNetwork } = getWalletState(store)
      let keyringSupportsEIP1559
      switch (selectedAccount.accountType) {
        case 'Primary':
        case 'Secondary':
        case 'Ledger':
          keyringSupportsEIP1559 = true
          break
        case 'Trezor':
          keyringSupportsEIP1559 = false
          break
        default:
          keyringSupportsEIP1559 = false
      }

      isEIP1559 = keyringSupportsEIP1559 && selectedNetwork.isEip1559
  }

  const { chainId } = await apiProxy.ethJsonRpcController.getChainId()

  let addResult
  const txData: TxData = {
    nonce: '',
    // Estimated by eth_tx_controller if value is '' for legacy transactions
    gasPrice: isEIP1559 ? '' : payload.gasPrice || '',
    // Estimated by eth_tx_controller if value is ''
    gasLimit: payload.gas || '',
    to: payload.to,
    value: payload.value,
    data: payload.data || []
  }

  if (isEIP1559) {
    const txData1559: TxData1559 = {
      baseData: txData,
      chainId,
      // Estimated by eth_tx_controller if value is ''
      maxPriorityFeePerGas: payload.maxPriorityFeePerGas || '',
      // Estimated by eth_tx_controller if value is ''
      maxFeePerGas: payload.maxFeePerGas || '',
      gasEstimation: undefined
    }
    addResult = await apiProxy.ethTxController.addUnapproved1559Transaction(txData1559, payload.from)
  } else {
    addResult = await apiProxy.ethTxController.addUnapprovedTransaction(txData, payload.from)
  }

  if (!addResult.success) {
    console.log(
      `Sending unapproved transaction failed: ` +
      `from=${payload.from} err=${addResult.errorMessage} txData=`, txData
    )
    return
  }

  // Refresh the transaction history of the origin account.
  await store.dispatch(refreshTransactionHistory(payload.from))
})

handler.on(WalletActions.sendERC20Transfer.getType(), async (store: Store, payload: ER20TransferParams) => {
  const apiProxy = getAPIProxy()
  const { data, success } = await apiProxy.ethTxController.makeERC20TransferData(payload.to, payload.value)
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
    data
  }))
})

handler.on(WalletActions.sendERC721TransferFrom.getType(), async (store: Store, payload: ERC721TransferFromParams) => {
  const apiProxy = getAPIProxy()
  const { data, success } = await apiProxy.ethTxController.makeERC721TransferFromData(payload.from, payload.to, payload.tokenId, payload.contractAddress)
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
    data
  }))
})

handler.on(WalletActions.approveERC20Allowance.getType(), async (store: Store, payload: ApproveERC20Params) => {
  const apiProxy = getAPIProxy()
  const { data, success } = await apiProxy.ethTxController.makeERC20ApproveData(payload.spenderAddress, payload.allowance)
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
    data
  }))
})

handler.on(WalletActions.approveTransaction.getType(), async (store: Store, txInfo: TransactionInfo) => {
  const apiProxy = getAPIProxy()
  const hardwareAccount = await findHardwareAccountInfo(txInfo.fromAddress)
  if (hardwareAccount && hardwareAccount.hardware) {
    const { success, message } = await apiProxy.ethTxController.approveHardwareTransaction(txInfo.id)
    if (success) {
      let deviceKeyring = await apiProxy.getKeyringsByType(hardwareAccount.hardware.vendor)
      if (deviceKeyring instanceof LedgerBridgeKeyring || deviceKeyring instanceof TrezorBridgeKeyring) {
        const { v, r, s } = await deviceKeyring.signTransaction(hardwareAccount.hardware.path, message.replace('0x', ''))
        await apiProxy.ethTxController.processLedgerSignature(txInfo.id, '0x' + v, r, s)
        await refreshWalletInfo(store)
      }
    }
    return
  }

  await apiProxy.ethTxController.approveTransaction(txInfo.id)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.rejectTransaction.getType(), async (store: Store, txInfo: TransactionInfo) => {
  const apiProxy = getAPIProxy()
  await apiProxy.ethTxController.rejectTransaction(txInfo.id)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.rejectAllTransactions.getType(), async (store) => {
  const state = getWalletState(store)
  const apiProxy = getAPIProxy()
  state.pendingTransactions.forEach(async (transaction) => {
    await apiProxy.ethTxController.rejectTransaction(transaction.id)
  })
  await refreshWalletInfo(store)
})

// fetchSwapQuoteFactory creates a handler function that can be used with
// both panel and page actions.
export const fetchSwapQuoteFactory = (
  setSwapQuote: SimpleActionCreator<SwapResponse>,
  setSwapError: SimpleActionCreator<SwapErrorResponse | undefined>
) => async (store: Store, payload: SwapParamsPayloadType) => {
  const swapController = getAPIProxy().swapController

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
    buyToken: toAsset.asset.contractAddress || toAsset.asset.symbol,
    sellToken: fromAsset.asset.contractAddress || fromAsset.asset.symbol,
    slippagePercentage: slippageTolerance.slippage / 100,
    gasPrice: ''
  }

  const quote = await (
    full ? swapController.getTransactionPayload(swapParams) : swapController.getPriceQuote(swapParams)
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

      const params = {
        from: accountAddress,
        to,
        value: toWeiHex(value, 0),
        gas: toWeiHex(estimatedGas, 0),
        data: hexStrToNumberArray(data)
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
  const assetPriceController = getAPIProxy().assetRatioController
  const basicEstimates = await assetPriceController.getGasOracle()
  if (!basicEstimates.estimation) {
    console.error(`Failed to fetch gas estimates`)
    return
  }

  store.dispatch(WalletActions.setGasEstimates(basicEstimates.estimation))
})

handler.on(WalletActions.updateUnapprovedTransactionGasFields.getType(), async (store: Store, payload: UpdateUnapprovedTransactionGasFieldsType) => {
  const apiProxy = getAPIProxy()

  const isEIP1559 = payload.maxPriorityFeePerGas !== undefined && payload.maxFeePerGas !== undefined

  if (isEIP1559) {
    const result = await apiProxy.ethTxController.setGasFeeAndLimitForUnapprovedTransaction(
      payload.txMetaId,
      payload.maxPriorityFeePerGas || '',
      payload.maxFeePerGas || '',
      payload.gasLimit
    )

    if (!result.success) {
      console.error(
        `Failed to update unapproved transaction: ` +
        `id=${payload.txMetaId} ` +
        `maxPriorityFeePerGas=${payload.maxPriorityFeePerGas}` +
        `maxFeePerGas=${payload.maxFeePerGas}` +
        `gasLimit=${payload.gasLimit}`
      )
    }
  }

  if (!isEIP1559 && payload.gasPrice) {
    const result = await apiProxy.ethTxController.setGasPriceAndLimitForUnapprovedTransaction(
      payload.txMetaId, payload.gasPrice, payload.gasLimit
    )

    if (!result.success) {
      console.error(
        `Failed to update unapproved transaction: ` +
        `id=${payload.txMetaId} ` +
        `gasPrice=${payload.gasPrice}` +
        `gasLimit=${payload.gasLimit}`
      )
    }
  }
})

handler.on(WalletActions.updateUnapprovedTransactionSpendAllowance.getType(), async (store: Store, payload: UpdateUnapprovedTransactionSpendAllowanceType) => {
  const apiProxy = getAPIProxy()

  const { data, success } = await apiProxy.ethTxController.makeERC20ApproveData(payload.spenderAddress, payload.allowance)
  if (!success) {
    console.error(
      `Failed making ERC20 approve data, spender: ${payload.spenderAddress}` +
      `, allowance: ${payload.allowance}`
    )
    return
  }

  const result = await apiProxy.ethTxController.setDataForUnapprovedTransaction(payload.txMetaId, data)
  if (!result.success) {
    console.error(
      `Failed to update unapproved transaction: ` +
      `id=${payload.txMetaId} ` +
      `allowance=${payload.allowance}`
    )
  }
})

handler.on(WalletActions.removeSitePermission.getType(), async (store: Store, payload: RemoveSitePermissionPayloadType) => {
  const braveWalletService = getAPIProxy().braveWalletService
  await braveWalletService.resetEthereumPermission(payload.origin, payload.account)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.transactionStatusChanged.getType(), async (store: Store, payload: TransactionStatusChanged) => {
  const status = payload.txInfo.txStatus
  if (status === TransactionStatus.Confirmed || status === TransactionStatus.Error) {
    await refreshBalancesPricesAndHistory(store)
  }
})

handler.on(WalletActions.retryTransaction.getType(), async (store: Store, payload: TransactionInfo) => {
  const { ethTxController } = getAPIProxy()
  const result = await ethTxController.retryTransaction(payload.id)
  if (!result.success) {
    console.error(
      `Retry transaction failed: ` +
      `id=${payload.id} ` +
      `err=${result.errorMessage}`
    )
  } else {
    // Refresh the transaction history of the origin account.
    await store.dispatch(refreshTransactionHistory(payload.fromAddress))
  }
})

handler.on(WalletActions.speedupTransaction.getType(), async (store: Store, payload: TransactionInfo) => {
  const { ethTxController } = getAPIProxy()
  const result = await ethTxController.speedupOrCancelTransaction(payload.id, false)
  if (!result.success) {
    console.error(
      `Speedup transaction failed: ` +
      `id=${payload.id} ` +
      `err=${result.errorMessage}`
    )
  } else {
    // Refresh the transaction history of the origin account.
    await store.dispatch(refreshTransactionHistory(payload.fromAddress))
  }
})

handler.on(WalletActions.cancelTransaction.getType(), async (store: Store, payload: TransactionInfo) => {
  const { ethTxController } = getAPIProxy()
  const result = await ethTxController.speedupOrCancelTransaction(payload.id, true)
  if (!result.success) {
    console.error(
      `Cancel transaction failed: ` +
      `id=${payload.id} ` +
      `err=${result.errorMessage}`
    )
  } else {
    // Refresh the transaction history of the origin account.
    await store.dispatch(refreshTransactionHistory(payload.fromAddress))
  }
})

export default handler.middleware
