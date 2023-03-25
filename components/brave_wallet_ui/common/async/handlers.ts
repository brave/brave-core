// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

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
  SelectedAccountChangedPayloadType,
  GetCoinMarketPayload,
  RetryTransactionPayload,
  SpeedupTransactionPayload,
  CancelTransactionPayload,
  UpdateUsetAssetType
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
  SPLTransferFromParams,
  NetworkFilterType
} from '../../constants/types'
import {
  AddAccountPayloadType,
  AddFilecoinAccountPayloadType
} from '../../page/constants/action_types'

// Utils

import getAPIProxy from './bridge'
import {
  refreshKeyringInfo,
  refreshTokenPriceHistory,
  refreshSitePermissions,
  refreshTransactionHistory,
  refreshBalances,
  refreshVisibleTokenInfo,
  refreshPrices,
  refreshPortfolioFilterOptions,
  sendEthTransaction,
  sendFilTransaction,
  sendSolTransaction,
  sendSPLTransaction,
  getNFTMetadata
} from './lib'
import { Store } from './types'
import InteractionNotifier from './interactionNotifier'
import {
  getCoinFromTxDataUnion,
  hasEIP1559Support
} from '../../utils/network-utils'
import { isSolanaTransaction, shouldReportTransactionP3A } from '../../utils/tx-utils'
import {
  getSelectedNetwork,
  getVisibleNetworksList,
  walletApi
} from '../slices/api.slice'
import { deserializeOrigin, makeSerializableOriginInfo } from '../../utils/model-serialization-utils'

const handler = new AsyncActionHandler()

const interactionNotifier = new InteractionNotifier()

function getWalletState (store: Store): WalletState {
  return store.getState().wallet
}

async function refreshBalancesPricesAndHistory (store: Store) {
  const state = getWalletState(store)
  await store.dispatch(refreshVisibleTokenInfo())
  await store.dispatch(refreshBalances())
  await store.dispatch(refreshPrices())
  await store.dispatch(refreshTokenPriceHistory(state.selectedPortfolioTimeline))
}

async function refreshWalletInfo (store: Store) {
  const apiProxy = getAPIProxy()

  await store.dispatch(refreshKeyringInfo())

  // refresh networks registry & selected network
  await store.dispatch(
    walletApi.endpoints.refreshNetworkInfo.initiate()
  ).unwrap()

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
  const walletInfo = (await walletHandler.getWalletInfo()).walletInfo
  if (state.accounts.length === walletInfo.accountInfos.length) {
    await store.dispatch(WalletActions.refreshAccountInfo(walletInfo))
  } else {
    await refreshWalletInfo(store)
  }
}

async function updateCoinAccountNetworkInfo (store: Store, coin: BraveWallet.CoinType) {
  const { accounts } = getWalletState(store)
  if (accounts.length === 0) {
    return
  }
  const { keyringService, jsonRpcService } = getAPIProxy()

  // Update Selected Coin & cached selected network
  await store
    .dispatch(walletApi.endpoints.setSelectedCoin.initiate(coin))
    .unwrap()

  // Updated Selected Account
  const { address: selectedAccountAddress } =
    coin === BraveWallet.CoinType.FIL
      ? await keyringService.getFilecoinSelectedAccount(
          (
            await jsonRpcService.getChainId(coin)
          ).chainId
        )
      : await keyringService.getSelectedAccount(coin)

  const defaultAccount =
    accounts.find(
      (account) => account.address === selectedAccountAddress
    ) || accounts[0]
  await store.dispatch(WalletActions.setSelectedAccount(defaultAccount))
  await store.dispatch(refreshTransactionHistory(defaultAccount.address))
}

handler.on(WalletActions.refreshBalancesAndPrices.type, async (store: Store) => {
  await store.dispatch(refreshVisibleTokenInfo())
  await store.dispatch(refreshBalances())
  await store.dispatch(refreshPrices())
})

handler.on(WalletActions.refreshNetworksAndTokens.type, async (store: Store) => {
  // refresh networks registry & selected network
  await store.dispatch(
    walletApi.endpoints.refreshNetworkInfo.initiate()
  ).unwrap()
  await store.dispatch(refreshVisibleTokenInfo())
  await store.dispatch(refreshBalances())
  await store.dispatch(refreshPrices())
  await store.dispatch(refreshPortfolioFilterOptions())
})

handler.on(WalletActions.initialize.type, async (store) => {
  // Initialize active origin state.
  const braveWalletService = getAPIProxy().braveWalletService
  const { originInfo } = await braveWalletService.getActiveOrigin()
  store.dispatch(WalletActions.activeOriginChanged(
    makeSerializableOriginInfo(originInfo)
  ))
  await refreshWalletInfo(store)
})

handler.on(WalletActions.keyringCreated.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.keyringRestored.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.keyringReset.type, async (store) => {
  window.location.reload()
})

handler.on(WalletActions.locked.type, async (store) => {
  interactionNotifier.stopWatchingForInteraction()
  await refreshWalletInfo(store)
})

handler.on(WalletActions.unlocked.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.backedUp.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.accountsChanged.type, async (store) => {
  await updateAccountInfo(store)
})

handler.on(WalletActions.chainChangedEvent.type, async (store: Store, payload: ChainChangedEventPayloadType) => {
  await updateCoinAccountNetworkInfo(store, payload.coin)
})

handler.on(
  WalletActions.selectAccount.type,
  async (
    store: Store,
    { address, coin }: Pick<WalletAccountType, 'address' | 'coin'>
  ) => {
    await store.dispatch(
      walletApi.endpoints.setSelectedAccount.initiate({ address, coin })
    )
  }
)

handler.on(WalletActions.selectedAccountChanged.type, async (store, payload: SelectedAccountChangedPayloadType) => {
  await updateCoinAccountNetworkInfo(store, payload.coin)
})

handler.on(WalletActions.defaultEthereumWalletChanged.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.defaultSolanaWalletChanged.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.defaultBaseCurrencyChanged.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.defaultBaseCryptocurrencyChanged.type, async (store) => {
  await refreshWalletInfo(store)
})

handler.on(WalletActions.lockWallet.type, async (store) => {
  const keyringService = getAPIProxy().keyringService
  keyringService.lock()
})

handler.on(WalletActions.unlockWallet.type, async (store: Store, payload: UnlockWalletPayloadType) => {
  const keyringService = getAPIProxy().keyringService
  const result = await keyringService.unlock(payload.password)
  store.dispatch(WalletActions.hasIncorrectPassword(!result.success))
})

handler.on(WalletActions.addFavoriteApp.type, async (store: Store, appItem: BraveWallet.AppItem) => {
  const walletHandler = getAPIProxy().walletHandler
  walletHandler.addFavoriteApp(appItem)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.removeFavoriteApp.type, async (store: Store, appItem: BraveWallet.AppItem) => {
  const walletHandler = getAPIProxy().walletHandler
  walletHandler.removeFavoriteApp(appItem)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.initialized.type, async (store: Store, payload: WalletInfo) => {
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
  const defaultCrypto = await braveWalletService.getDefaultBaseCryptocurrency()
  const defaultCurrencies = {
    fiat: defaultFiat.currency,
    crypto: defaultCrypto.cryptocurrency
  }
  store.dispatch(WalletActions.defaultCurrenciesUpdated(defaultCurrencies))
  // Fetch Balances and Prices
  if (!state.isWalletLocked && state.isWalletCreated) {
    // refresh networks registry & selected network
    await store.dispatch(
      walletApi.endpoints.refreshNetworkInfo.initiate()
    ).unwrap()
    await store.dispatch(refreshVisibleTokenInfo())
    await store.dispatch(refreshBalances())
    await store.dispatch(refreshPortfolioFilterOptions())
    await store.dispatch(refreshPrices())
    await store.dispatch(refreshTokenPriceHistory(state.selectedPortfolioTimeline))
    await braveWalletService.discoverAssetsOnAllSupportedChains()
  }

  // This can be 0 when the wallet is locked
  if (payload.selectedAccount) {
    await store.dispatch(refreshTransactionHistory(payload.selectedAccount))
  }
})

handler.on(WalletActions.getAllTokensList.type, async (store) => {
  const api = getAPIProxy()
  const networkList = await getVisibleNetworksList(api)
  const { blockchainRegistry } = api
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

handler.on(WalletActions.addUserAsset.type, async (store: Store, payload: BraveWallet.BlockchainToken) => {
  const { braveWalletService } = getAPIProxy()

  if (payload.isErc721 || payload.isNft) {
    const result = await getNFTMetadata(payload)
    if (!result?.error) {
      const response = result?.response && JSON.parse(result.response)
      payload.logo = response.image || payload.logo
    }
  }

  const result = await braveWalletService.addUserAsset(payload)

  // Refresh balances here for adding ERC721 tokens if result is successful
  if ((payload.isErc721 || payload.isNft) && result.success) {
    refreshBalancesPricesAndHistory(store)
  }
  store.dispatch(WalletActions.addUserAssetError(!result.success))
})

handler.on(WalletActions.updateUserAsset.type, async (store: Store, payload: UpdateUsetAssetType) => {
  const { braveWalletService } = getAPIProxy()
  const { existing, updated } = payload
  // fetch NFT metadata if tokenId or contract address has changed
  if ((updated.isNft || updated.isErc721) && (updated.tokenId !== existing.tokenId || updated.contractAddress !== existing.contractAddress)) {
    const result = await getNFTMetadata(updated)
    if (!result?.error) {
      try {
        const nftMetadata = result?.response && JSON.parse(result.response)
        updated.logo = nftMetadata?.image || ''
      } catch (error) {
        console.error(error)
      }
    }
  }

  const deleteResult = await braveWalletService.removeUserAsset(existing)
  if (deleteResult.success) {
    const addResult = await braveWalletService.addUserAsset(updated)
    if (addResult.success) {
      refreshBalancesPricesAndHistory(store)
      refreshVisibleTokenInfo()
    }
  }
})

handler.on(WalletActions.removeUserAsset.type, async (store: Store, payload: BraveWallet.BlockchainToken) => {
  const { braveWalletService } = getAPIProxy()
  await braveWalletService.removeUserAsset(payload)
})

handler.on(WalletActions.setUserAssetVisible.type, async (store: Store, payload: SetUserAssetVisiblePayloadType) => {
  const { braveWalletService } = getAPIProxy()
  await braveWalletService.setUserAssetVisible(payload.token, payload.isVisible)
})

handler.on(WalletActions.refreshBalancesAndPriceHistory.type, async (store: Store) => {
  refreshBalancesPricesAndHistory(store)
})

handler.on(WalletActions.selectPortfolioTimeline.type, async (store: Store, payload: BraveWallet.AssetPriceTimeframe) => {
  store.dispatch(WalletActions.portfolioTimelineUpdated(payload))
  await store.dispatch(refreshTokenPriceHistory(payload))
})

handler.on(WalletActions.sendTransaction.type, async (
  store: Store,
  payload:
    | Omit<SendEthTransactionParams, 'hasEIP1559Support'>
    | SendFilTransactionParams
    | SendSolTransactionParams
) => {
  const { wallet: walletState } = store.getState()
  const selectedNetwork = await getSelectedNetwork(getAPIProxy())

  let addResult
  if (payload.coin === BraveWallet.CoinType.ETH) {
    addResult = await sendEthTransaction({
      ...payload,
      hasEIP1559Support:
        !!selectedNetwork &&
        !!walletState.selectedAccount &&
        hasEIP1559Support(
          walletState.selectedAccount.accountType,
          selectedNetwork
        )
    })
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

handler.on(WalletActions.sendSPLTransfer.type, async (store: Store, payload: SPLTransferFromParams) => {
  const { solanaTxManagerProxy } = getAPIProxy()
  const value = await solanaTxManagerProxy.makeTokenProgramTransferTxData(payload.splTokenMintAddress, payload.from, payload.to, BigInt(payload.value))
  if (!value.txData) {
    console.log('Failed making SPL transfer data, to: ', payload.to, ', value: ', payload.value)
    return
  }
  await sendSPLTransaction(value.txData)
  await store.dispatch(refreshTransactionHistory(payload.from))
})

handler.on(WalletActions.sendERC20Transfer.type, async (store: Store, payload: ER20TransferParams) => {
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

handler.on(WalletActions.sendERC721TransferFrom.type, async (store: Store, payload: ERC721TransferFromParams) => {
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

handler.on(WalletActions.approveERC20Allowance.type, async (store: Store, payload: ApproveERC20Params) => {
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

handler.on(WalletActions.approveTransaction.type, async (store: Store, txInfo: BraveWallet.TransactionInfo) => {
  const api = getAPIProxy()
  const { txService, braveWalletP3A } = api
  const coin = getCoinFromTxDataUnion(txInfo.txDataUnion)
  const result = await txService.approveTransaction(coin, txInfo.id)
  const error = result.errorUnion.providerError ?? result.errorUnion.solanaProviderError ?? result.errorUnion.filecoinProviderError
  if (error !== BraveWallet.ProviderError.kSuccess) {
    await store.dispatch(WalletActions.setTransactionProviderError({
      transaction: txInfo,
      providerError: {
        code: error,
        message: result.errorMessage
      } as TransactionProviderError
    }))
  } else {
    const selectedNetwork = await getSelectedNetwork(api)
    if (selectedNetwork && shouldReportTransactionP3A(txInfo, selectedNetwork, coin)) {
      braveWalletP3A.reportTransactionSent(coin, true)
    }
  }

  await store.dispatch(refreshTransactionHistory(txInfo.fromAddress))
})

handler.on(WalletActions.rejectTransaction.type, async (store: Store, txInfo: BraveWallet.TransactionInfo) => {
  const apiProxy = getAPIProxy()
  const coin = getCoinFromTxDataUnion(txInfo.txDataUnion)
  await apiProxy.txService.rejectTransaction(coin, txInfo.id)
  await store.dispatch(refreshTransactionHistory(txInfo.fromAddress))
})

handler.on(WalletActions.rejectAllTransactions.type, async (store) => {
  const state = getWalletState(store)
  const apiProxy = getAPIProxy()
  state.pendingTransactions.forEach(async (transaction) => {
    const coin = getCoinFromTxDataUnion(transaction.txDataUnion)
    await apiProxy.txService.rejectTransaction(coin, transaction.id)
  })
  await refreshWalletInfo(store)
})

handler.on(WalletActions.refreshGasEstimates.type, async (store: Store, txInfo: BraveWallet.TransactionInfo) => {
  const { selectedAccount} = getWalletState(store)
  const api = getAPIProxy()
  const { ethTxManagerProxy, solanaTxManagerProxy } = api
  const selectedNetwork = await getSelectedNetwork(api)

  if (isSolanaTransaction(txInfo)) {
    const { fee, errorMessage } = await solanaTxManagerProxy.getEstimatedTxFee(
      txInfo.id
    )
    if (!fee) {
      console.error('Failed to fetch SOL Fee estimates: ' + errorMessage)
      store.dispatch(WalletActions.setHasFeeEstimatesError(true))
      return
    }
    store.dispatch(WalletActions.setSolFeeEstimates({ fee }))
    return
  }

  if (
    selectedNetwork &&
    selectedAccount &&
    !hasEIP1559Support(selectedAccount.accountType, selectedNetwork)
  ) {
    store.dispatch(WalletActions.setHasFeeEstimatesError(false))
    return
  }

  const { estimation } = await ethTxManagerProxy.getGasEstimation1559()
  if (!estimation) {
    console.error('Failed to fetch gas estimates')
    store.dispatch(WalletActions.setHasFeeEstimatesError(true))
    return
  }

  store.dispatch(WalletActions.setGasEstimates(estimation))
})

handler.on(WalletActions.updateUnapprovedTransactionGasFields.type, async (store: Store, payload: UpdateUnapprovedTransactionGasFieldsType) => {
  const apiProxy = getAPIProxy()

  const isEIP1559 = payload.maxPriorityFeePerGas !== undefined && payload.maxFeePerGas !== undefined

  if (isEIP1559) {
    const result = await apiProxy.ethTxManagerProxy.setGasFeeAndLimitForUnapprovedTransaction(
      payload.txMetaId,
      payload.maxPriorityFeePerGas || '0x0',
      payload.maxFeePerGas || '0x0',
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

handler.on(WalletActions.updateUnapprovedTransactionSpendAllowance.type, async (store: Store, payload: UpdateUnapprovedTransactionSpendAllowanceType) => {
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

handler.on(WalletActions.updateUnapprovedTransactionNonce.type, async (store: Store, payload: UpdateUnapprovedTransactionNonceType) => {
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

handler.on(WalletActions.removeSitePermission.type, async (store: Store, payload: RemoveSitePermissionPayloadType) => {
  const braveWalletService = getAPIProxy().braveWalletService
  await braveWalletService.resetPermission(payload.coin, deserializeOrigin(payload.origin), payload.account)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.addSitePermission.type, async (store: Store, payload: AddSitePermissionPayloadType) => {
  const braveWalletService = getAPIProxy().braveWalletService
  await braveWalletService.addPermission(payload.coin, deserializeOrigin(payload.origin), payload.account)
  await refreshWalletInfo(store)
})

handler.on(WalletActions.transactionStatusChanged.type, async (store: Store, payload: TransactionStatusChanged) => {
  await store.dispatch(
    walletApi.endpoints.transactionStatusChanged.initiate({
      coinType: getCoinFromTxDataUnion(payload.txInfo.txDataUnion),
      fromAddress: payload.txInfo.fromAddress,
      txStatus: payload.txInfo.txStatus,
      id: payload.txInfo.id
    })
  )
  const status = payload.txInfo.txStatus
  if (status === BraveWallet.TransactionStatus.Confirmed || status === BraveWallet.TransactionStatus.Error) {
    await refreshBalancesPricesAndHistory(store)
  }
})

handler.on(
  WalletActions.retryTransaction.type,
  async (store: Store, payload: RetryTransactionPayload) => {
    const { txService } = getAPIProxy()
    const result = await txService.retryTransaction(
      payload.coinType,
      payload.transactionId
    )
    if (!result.success) {
      console.error(
        'Retry transaction failed: ' +
          `id=${payload.transactionId} ` +
          `err=${result.errorMessage}`
      )
    } else {
      // Refresh the transaction history of the origin account.
      await store.dispatch(refreshTransactionHistory(payload.fromAddress))
    }
  }
)

handler.on(
  WalletActions.speedupTransaction.type,
  async (store: Store, payload: SpeedupTransactionPayload) => {
    const { txService } = getAPIProxy()
    const result = await txService.speedupOrCancelTransaction(
      payload.coinType,
      payload.transactionId,
      false
    )
    if (!result.success) {
      console.error(
        'Speedup transaction failed: ' +
          `id=${payload.transactionId} ` +
          `err=${result.errorMessage}`
      )
    } else {
      // Refresh the transaction history of the origin account.
      await store.dispatch(refreshTransactionHistory(payload.fromAddress))
    }
  }
)

handler.on(
  WalletActions.cancelTransaction.type,
  async (store: Store, payload: CancelTransactionPayload) => {
    const { txService } = getAPIProxy()
    const result = await txService.speedupOrCancelTransaction(
      payload.coinType,
      payload.transactionId,
      true
    )
    if (!result.success) {
      console.error(
        'Cancel transaction failed: ' +
          `id=${payload.transactionId} ` +
          `err=${result.errorMessage}`
      )
    } else {
      // Refresh the transaction history of the origin account.
      await store.dispatch(refreshTransactionHistory(payload.fromAddress))
    }
  }
)

handler.on(WalletActions.expandWalletNetworks.type, async (store) => {
  chrome.tabs.create({ url: 'chrome://settings/wallet/networks' }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
})

handler.on(WalletActions.getCoinMarkets.type, async (store: Store, payload: GetCoinMarketPayload) => {
  const assetRatioService = getAPIProxy().assetRatioService
  const result = await assetRatioService.getCoinMarkets(payload.vsAsset, payload.limit)
  store.dispatch(WalletActions.setCoinMarkets(result))
})

handler.on(WalletActions.setSelectedNetworkFilter.type, async (store: Store, payload: NetworkFilterType) => {
  const state = getWalletState(store)
  const { selectedPortfolioTimeline } = state
  await store.dispatch(refreshTokenPriceHistory(selectedPortfolioTimeline))
})

handler.on(WalletActions.setSelectedAccountFilterItem.type, async (store: Store, payload: string) => {
  const state = getWalletState(store)
  const { selectedPortfolioTimeline } = state
  await store.dispatch(refreshTokenPriceHistory(selectedPortfolioTimeline))
})

handler.on(WalletActions.addAccount.type, async (_store: Store, payload: AddAccountPayloadType) => {
  const { keyringService } = getAPIProxy()
  const result = await keyringService.addAccount(payload.accountName, payload.coin)
  return result.success
})

handler.on(WalletActions.addFilecoinAccount.type, async (_store: Store, payload: AddFilecoinAccountPayloadType) => {
  const { keyringService } = getAPIProxy()
  const result = await keyringService.addFilecoinAccount(payload.accountName, payload.network)
  return result.success
})

handler.on(WalletActions.getOnRampCurrencies.type, async (store: Store) => {
  const { blockchainRegistry } = getAPIProxy()
  const currencies = (await blockchainRegistry.getOnRampCurrencies()).currencies
  await store.dispatch(WalletActions.setOnRampCurrencies(currencies))
})

export default handler.middleware
